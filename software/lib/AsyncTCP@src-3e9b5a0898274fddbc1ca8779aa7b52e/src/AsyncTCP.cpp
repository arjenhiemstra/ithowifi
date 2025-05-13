// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2025 Hristo Gochkov, Mathieu Carbou, Emil Muratov

#include "AsyncTCP.h"

#include <esp_log.h>

#ifdef ARDUINO
#include <esp32-hal.h>
#include <esp32-hal-log.h>
#if (ESP_IDF_VERSION_MAJOR >= 5)
#include <NetworkInterface.h>
#endif
#else
#include "esp_timer.h"
#define log_e(...) ESP_LOGE(__FILE__, __VA_ARGS__)
#define log_w(...) ESP_LOGW(__FILE__, __VA_ARGS__)
#define log_i(...) ESP_LOGI(__FILE__, __VA_ARGS__)
#define log_d(...) ESP_LOGD(__FILE__, __VA_ARGS__)
#define log_v(...) ESP_LOGV(__FILE__, __VA_ARGS__)
static unsigned long millis() {
  return (unsigned long)(esp_timer_get_time() / 1000ULL);
}
#endif

extern "C" {
#include "lwip/dns.h"
#include "lwip/err.h"
#include "lwip/inet.h"
#include "lwip/opt.h"
#include "lwip/tcp.h"
#include "lwip/tcpip.h"
}

#if CONFIG_ASYNC_TCP_USE_WDT
#include "esp_task_wdt.h"
#endif

// Required for:
// https://github.com/espressif/arduino-esp32/blob/3.0.3/libraries/Network/src/NetworkInterface.cpp#L37-L47

// https://github.com/espressif/arduino-esp32/issues/10526
namespace {
#ifdef CONFIG_LWIP_TCPIP_CORE_LOCKING
struct tcp_core_guard {
  bool do_lock;
  inline tcp_core_guard() : do_lock(!sys_thread_tcpip(LWIP_CORE_LOCK_QUERY_HOLDER)) {
    if (do_lock) {
      LOCK_TCPIP_CORE();
    }
  }
  inline ~tcp_core_guard() {
    if (do_lock) {
      UNLOCK_TCPIP_CORE();
    }
  }
  tcp_core_guard(const tcp_core_guard &) = delete;
  tcp_core_guard(tcp_core_guard &&) = delete;
  tcp_core_guard &operator=(const tcp_core_guard &) = delete;
  tcp_core_guard &operator=(tcp_core_guard &&) = delete;
} __attribute__((unused));
#else   // CONFIG_LWIP_TCPIP_CORE_LOCKING
struct tcp_core_guard {
} __attribute__((unused));
#endif  // CONFIG_LWIP_TCPIP_CORE_LOCKING
}  // anonymous namespace

#define INVALID_CLOSED_SLOT -1

/*
  TCP poll interval is specified in terms of the TCP coarse timer interval, which is called twice a second
  https://github.com/espressif/esp-lwip/blob/2acf959a2bb559313cd2bf9306c24612ba3d0e19/src/core/tcp.c#L1895
*/
#define CONFIG_ASYNC_TCP_POLL_TIMER 1

/*
 * TCP/IP Event Task
 * */

typedef enum {
  LWIP_TCP_SENT,
  LWIP_TCP_RECV,
  LWIP_TCP_FIN,
  LWIP_TCP_ERROR,
  LWIP_TCP_POLL,
  LWIP_TCP_CLEAR,
  LWIP_TCP_ACCEPT,
  LWIP_TCP_CONNECTED,
  LWIP_TCP_DNS
} lwip_tcp_event_t;

typedef struct {
  lwip_tcp_event_t event;
  AsyncClient *client;
  union {
    struct {
      tcp_pcb *pcb;
      int8_t err;
    } connected;
    struct {
      int8_t err;
    } error;
    struct {
      tcp_pcb *pcb;
      uint16_t len;
    } sent;
    struct {
      tcp_pcb *pcb;
      pbuf *pb;
      int8_t err;
    } recv;
    struct {
      tcp_pcb *pcb;
      int8_t err;
    } fin;
    struct {
      tcp_pcb *pcb;
    } poll;
    struct {
      AsyncServer *server;
    } accept;
    struct {
      const char *name;
      ip_addr_t addr;
    } dns;
  };
} lwip_tcp_event_packet_t;

// Detail class for interacting with AsyncClient internals, but without exposing the API
class AsyncTCP_detail {
public:
  // Helper functions
  static void __attribute__((visibility("internal"))) handle_async_event(lwip_tcp_event_packet_t *event);

  // LwIP TCP event callbacks that (will) require privileged access
  static int8_t __attribute__((visibility("internal"))) tcp_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *pb, int8_t err);
  static int8_t __attribute__((visibility("internal"))) tcp_sent(void *arg, struct tcp_pcb *pcb, uint16_t len);
  static void __attribute__((visibility("internal"))) tcp_error(void *arg, int8_t err);
  static int8_t __attribute__((visibility("internal"))) tcp_poll(void *arg, struct tcp_pcb *pcb);
  static int8_t __attribute__((visibility("internal"))) tcp_accept(void *arg, tcp_pcb *pcb, int8_t err);
};

static QueueHandle_t _async_queue = NULL;
static TaskHandle_t _async_service_task_handle = NULL;

static SemaphoreHandle_t _slots_lock = NULL;
static const int _number_of_closed_slots = CONFIG_LWIP_MAX_ACTIVE_TCP;
static uint32_t _closed_slots[_number_of_closed_slots];
static uint32_t _closed_index = []() {
  _slots_lock = xSemaphoreCreateBinary();
  configASSERT(_slots_lock);  // Add sanity check
  xSemaphoreGive(_slots_lock);
  for (int i = 0; i < _number_of_closed_slots; ++i) {
    _closed_slots[i] = 1;
  }
  return 1;
}();

static inline bool _init_async_event_queue() {
  if (!_async_queue) {
    _async_queue = xQueueCreate(CONFIG_ASYNC_TCP_QUEUE_SIZE, sizeof(lwip_tcp_event_packet_t *));
    if (!_async_queue) {
      return false;
    }
  }
  return true;
}

static inline bool _send_async_event(lwip_tcp_event_packet_t **e, TickType_t wait = portMAX_DELAY) {
  return _async_queue && xQueueSend(_async_queue, e, wait) == pdPASS;
}

static inline bool _prepend_async_event(lwip_tcp_event_packet_t **e, TickType_t wait = portMAX_DELAY) {
  return _async_queue && xQueueSendToFront(_async_queue, e, wait) == pdPASS;
}

static inline bool _get_async_event(lwip_tcp_event_packet_t **e) {
  while (true) {
    if (!_async_queue) {
      break;
    }

#if CONFIG_ASYNC_TCP_USE_WDT
    // need to return periodically to feed the dog
    if (xQueueReceive(_async_queue, e, pdMS_TO_TICKS(1000)) != pdPASS) {
      break;
    }
#else
    if (xQueueReceive(_async_queue, e, portMAX_DELAY) != pdPASS) {
      break;
    }
#endif

    if ((*e)->event != LWIP_TCP_POLL) {
      return true;
    }

    /*
      Let's try to coalesce two (or more) consecutive poll events into one
      this usually happens with poor implemented user-callbacks that are runs too long and makes poll events to stack in the queue
      if consecutive user callback for a same connection runs longer that poll time then it will fill the queue with events until it deadlocks.
      This is a workaround to mitigate such poor designs and won't let other events/connections to starve the task time.
      It won't be effective if user would run multiple simultaneous long running callbacks due to message interleaving.
      todo: implement some kind of fair dequeuing or (better) simply punish user for a bad designed callbacks by resetting hog connections
    */
    lwip_tcp_event_packet_t *next_pkt = NULL;
    while (xQueuePeek(_async_queue, &next_pkt, 0) == pdPASS) {
      // if the next event that will come is a poll event for the same connection, we can discard it and continue
      if (next_pkt->client == (*e)->client && next_pkt->event == LWIP_TCP_POLL) {
        if (xQueueReceive(_async_queue, &next_pkt, 0) == pdPASS) {
          free(next_pkt);
          next_pkt = NULL;
          log_d("coalescing polls, network congestion or async callbacks might be too slow!");
          continue;
        }
      }

      // quit while loop if next incoming event can't be discarded (not a poll event)
      break;
    }

    /*
      now we have to decide if to proceed with poll callback handler or discard it?
      poor designed apps using asynctcp without proper dataflow control could flood the queue with interleaved pool/ack events.
      I.e. on each poll app would try to generate more data to send, which in turn results in additional ack event triggering chain effect
      for long connections. Or poll callback could take long time starving other connections. Anyway our goal is to keep the queue length
      grows under control (if possible) and poll events are the safest to discard.
      Let's discard poll events processing using linear-increasing probability curve when queue size grows over 3/4
      Poll events are periodic and connection could get another chance next time
    */
    if (uxQueueMessagesWaiting(_async_queue) > (rand() % CONFIG_ASYNC_TCP_QUEUE_SIZE / 4 + CONFIG_ASYNC_TCP_QUEUE_SIZE * 3 / 4)) {
      free(*e);
      *e = NULL;
      log_d("discarding poll due to queue congestion");
      continue;  // continue main loop to dequeue next event which we know is not a poll event
    }
    return true;  // queue not nearly full, caller can process the poll event
  }
  return false;
}

static bool _remove_events_for_client(AsyncClient *client) {
  if (!_async_queue) {
    return false;
  }

  lwip_tcp_event_packet_t *first_packet = NULL;
  lwip_tcp_event_packet_t *packet = NULL;

  // figure out which is the first non-matching packet so we can keep the order
  while (!first_packet) {
    if (xQueueReceive(_async_queue, &first_packet, 0) != pdPASS) {
      return false;
    }
    // discard packet if matching
    if ((uintptr_t)first_packet->client == (uintptr_t)client) {
      free(first_packet);
      first_packet = NULL;
    } else if (xQueueSend(_async_queue, &first_packet, 0) != pdPASS) {
      // try to return first packet to the back of the queue
      // we can't wait here if queue is full, because this call has been done from the only consumer task of this queue
      // otherwise it would deadlock, we have to discard the event
      free(first_packet);
      first_packet = NULL;
      return false;
    }
  }

  while (xQueuePeek(_async_queue, &packet, 0) == pdPASS && packet != first_packet) {
    if (xQueueReceive(_async_queue, &packet, 0) != pdPASS) {
      return false;
    }
    if ((uintptr_t)packet->client == (uintptr_t)client) {
      // remove matching event
      free(packet);
      packet = NULL;
      // otherwise try to requeue it
    } else if (xQueueSend(_async_queue, &packet, 0) != pdPASS) {
      // we can't wait here if queue is full, because this call has been done from the only consumer task of this queue
      // otherwise it would deadlock, we have to discard the event
      free(packet);
      packet = NULL;
      return false;
    }
  }
  return true;
}

void AsyncTCP_detail::handle_async_event(lwip_tcp_event_packet_t *e) {
  if (e->client == NULL) {
    // do nothing when arg is NULL
    // ets_printf("event arg == NULL: 0x%08x\n", e->recv.pcb);
  } else if (e->event == LWIP_TCP_CLEAR) {
    _remove_events_for_client(e->client);
  } else if (e->event == LWIP_TCP_RECV) {
    // ets_printf("-R: 0x%08x\n", e->recv.pcb);
    e->client->_recv(e->recv.pcb, e->recv.pb, e->recv.err);
  } else if (e->event == LWIP_TCP_FIN) {
    // ets_printf("-F: 0x%08x\n", e->fin.pcb);
    e->client->_fin(e->fin.pcb, e->fin.err);
  } else if (e->event == LWIP_TCP_SENT) {
    // ets_printf("-S: 0x%08x\n", e->sent.pcb);
    e->client->_sent(e->sent.pcb, e->sent.len);
  } else if (e->event == LWIP_TCP_POLL) {
    // ets_printf("-P: 0x%08x\n", e->poll.pcb);
    e->client->_poll(e->poll.pcb);
  } else if (e->event == LWIP_TCP_ERROR) {
    // ets_printf("-E: 0x%08x %d\n", e->client, e->error.err);
    e->client->_error(e->error.err);
  } else if (e->event == LWIP_TCP_CONNECTED) {
    // ets_printf("C: 0x%08x 0x%08x %d\n", e->client, e->connected.pcb, e->connected.err);
    e->client->_connected(e->connected.pcb, e->connected.err);
  } else if (e->event == LWIP_TCP_ACCEPT) {
    // ets_printf("A: 0x%08x 0x%08x\n", e->client, e->accept.client);
    e->accept.server->_accepted(e->client);
  } else if (e->event == LWIP_TCP_DNS) {
    // ets_printf("D: 0x%08x %s = %s\n", e->client, e->dns.name, ipaddr_ntoa(&e->dns.addr));
    e->client->_dns_found(&e->dns.addr);
  }
  free((void *)(e));
}

static void _async_service_task(void *pvParameters) {
#if CONFIG_ASYNC_TCP_USE_WDT
  if (esp_task_wdt_add(NULL) != ESP_OK) {
    log_w("Failed to add async task to WDT");
  }
#endif
  lwip_tcp_event_packet_t *packet = NULL;
  for (;;) {
    if (_get_async_event(&packet)) {
      AsyncTCP_detail::handle_async_event(packet);
    }
#if CONFIG_ASYNC_TCP_USE_WDT
    esp_task_wdt_reset();
#endif
  }
#if CONFIG_ASYNC_TCP_USE_WDT
  esp_task_wdt_delete(NULL);
#endif
  vTaskDelete(NULL);
  _async_service_task_handle = NULL;
}
/*
static void _stop_async_task(){
    if(_async_service_task_handle){
        vTaskDelete(_async_service_task_handle);
        _async_service_task_handle = NULL;
    }
}
*/

static bool customTaskCreateUniversal(
  TaskFunction_t pxTaskCode, const char *const pcName, const uint32_t usStackDepth, void *const pvParameters, UBaseType_t uxPriority,
  TaskHandle_t *const pxCreatedTask, const BaseType_t xCoreID
) {
#ifndef CONFIG_FREERTOS_UNICORE
  if (xCoreID >= 0 && xCoreID < 2) {
    return xTaskCreatePinnedToCore(pxTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pxCreatedTask, xCoreID);
  } else {
#endif
    return xTaskCreate(pxTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pxCreatedTask);
#ifndef CONFIG_FREERTOS_UNICORE
  }
#endif
}

static bool _start_async_task() {
  if (!_init_async_event_queue()) {
    return false;
  }
  if (!_async_service_task_handle) {
    customTaskCreateUniversal(
      _async_service_task, "async_tcp", CONFIG_ASYNC_TCP_STACK_SIZE, NULL, CONFIG_ASYNC_TCP_PRIORITY, &_async_service_task_handle, CONFIG_ASYNC_TCP_RUNNING_CORE
    );
    if (!_async_service_task_handle) {
      return false;
    }
  }
  return true;
}

/*
 * LwIP Callbacks
 * */

static int8_t _tcp_clear_events(AsyncClient *client) {
  lwip_tcp_event_packet_t *e = (lwip_tcp_event_packet_t *)malloc(sizeof(lwip_tcp_event_packet_t));
  if (!e) {
    log_e("Failed to allocate event packet");
    return ERR_MEM;
  }
  e->event = LWIP_TCP_CLEAR;
  e->client = client;
  if (!_prepend_async_event(&e)) {
    free((void *)(e));
    return ERR_TIMEOUT;
  }
  return ERR_OK;
}

static int8_t _tcp_connected(void *arg, tcp_pcb *pcb, int8_t err) {
  // ets_printf("+C: 0x%08x\n", pcb);
  lwip_tcp_event_packet_t *e = (lwip_tcp_event_packet_t *)malloc(sizeof(lwip_tcp_event_packet_t));
  if (!e) {
    log_e("Failed to allocate event packet");
    return ERR_MEM;
  }
  e->event = LWIP_TCP_CONNECTED;
  e->client = reinterpret_cast<AsyncClient *>(arg);
  e->connected.pcb = pcb;
  e->connected.err = err;
  if (!_prepend_async_event(&e)) {
    free((void *)(e));
    return ERR_TIMEOUT;
  }
  return ERR_OK;
}

int8_t AsyncTCP_detail::tcp_poll(void *arg, struct tcp_pcb *pcb) {
  // throttle polling events queueing when event queue is getting filled up, let it handle _onack's
  // log_d("qs:%u", uxQueueMessagesWaiting(_async_queue));
  if (uxQueueMessagesWaiting(_async_queue) > (rand() % CONFIG_ASYNC_TCP_QUEUE_SIZE / 2 + CONFIG_ASYNC_TCP_QUEUE_SIZE / 4)) {
    log_d("throttling");
    return ERR_OK;
  }

  // ets_printf("+P: 0x%08x\n", pcb);
  lwip_tcp_event_packet_t *e = (lwip_tcp_event_packet_t *)malloc(sizeof(lwip_tcp_event_packet_t));
  if (!e) {
    log_e("Failed to allocate event packet");
    return ERR_MEM;
  }
  e->event = LWIP_TCP_POLL;
  e->client = reinterpret_cast<AsyncClient *>(arg);
  e->poll.pcb = pcb;
  // poll events are not critical 'cause those are repetitive, so we may not wait the queue in any case
  if (!_send_async_event(&e, 0)) {
    free((void *)(e));
    return ERR_TIMEOUT;
  }
  return ERR_OK;
}

int8_t AsyncTCP_detail::tcp_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *pb, int8_t err) {
  lwip_tcp_event_packet_t *e = (lwip_tcp_event_packet_t *)malloc(sizeof(lwip_tcp_event_packet_t));
  if (!e) {
    log_e("Failed to allocate event packet");
    return ERR_MEM;
  }
  e->client = reinterpret_cast<AsyncClient *>(arg);
  if (pb) {
    // ets_printf("+R: 0x%08x\n", pcb);
    e->event = LWIP_TCP_RECV;
    e->recv.pcb = pcb;
    e->recv.pb = pb;
    e->recv.err = err;
  } else {
    // ets_printf("+F: 0x%08x\n", pcb);
    e->event = LWIP_TCP_FIN;
    e->fin.pcb = pcb;
    e->fin.err = err;
    // close the PCB in LwIP thread
    reinterpret_cast<AsyncClient *>(arg)->_lwip_fin(e->fin.pcb, e->fin.err);
  }
  if (!_send_async_event(&e)) {
    free((void *)(e));
    return ERR_TIMEOUT;
  }
  return ERR_OK;
}

int8_t AsyncTCP_detail::tcp_sent(void *arg, struct tcp_pcb *pcb, uint16_t len) {
  // ets_printf("+S: 0x%08x\n", pcb);
  lwip_tcp_event_packet_t *e = (lwip_tcp_event_packet_t *)malloc(sizeof(lwip_tcp_event_packet_t));
  if (!e) {
    log_e("Failed to allocate event packet");
    return ERR_MEM;
  }
  e->event = LWIP_TCP_SENT;
  e->client = reinterpret_cast<AsyncClient *>(arg);
  e->sent.pcb = pcb;
  e->sent.len = len;
  if (!_send_async_event(&e)) {
    free((void *)(e));
    return ERR_TIMEOUT;
  }
  return ERR_OK;
}

void AsyncTCP_detail::tcp_error(void *arg, int8_t err) {
  // ets_printf("+E: 0x%08x\n", arg);
  AsyncClient *client = reinterpret_cast<AsyncClient *>(arg);
  if (client && client->_pcb) {
    tcp_arg(client->_pcb, NULL);
    if (client->_pcb->state == LISTEN) {
      ::tcp_sent(client->_pcb, NULL);
      ::tcp_recv(client->_pcb, NULL);
      ::tcp_err(client->_pcb, NULL);
      ::tcp_poll(client->_pcb, NULL, 0);
    }
    client->_pcb = nullptr;
    client->_free_closed_slot();
  }

  // enqueue event to be processed in the async task for the user callback
  lwip_tcp_event_packet_t *e = (lwip_tcp_event_packet_t *)malloc(sizeof(lwip_tcp_event_packet_t));
  if (!e) {
    log_e("Failed to allocate event packet");
    return;
  }
  e->event = LWIP_TCP_ERROR;
  e->client = client;
  e->error.err = err;
  if (!_send_async_event(&e)) {
    ::free((void *)(e));
  }
}

static void _tcp_dns_found(const char *name, struct ip_addr *ipaddr, void *arg) {
  lwip_tcp_event_packet_t *e = (lwip_tcp_event_packet_t *)malloc(sizeof(lwip_tcp_event_packet_t));
  if (!e) {
    log_e("Failed to allocate event packet");
    return;
  }
  // ets_printf("+DNS: name=%s ipaddr=0x%08x arg=%x\n", name, ipaddr, arg);
  e->event = LWIP_TCP_DNS;
  e->client = reinterpret_cast<AsyncClient *>(arg);
  e->dns.name = name;
  if (ipaddr) {
    memcpy(&e->dns.addr, ipaddr, sizeof(struct ip_addr));
  } else {
    memset(&e->dns.addr, 0, sizeof(e->dns.addr));
  }
  if (!_send_async_event(&e)) {
    free((void *)(e));
  }
}

/*
 * TCP/IP API Calls
 * */

#include "lwip/priv/tcpip_priv.h"

typedef struct {
  struct tcpip_api_call_data call;
  tcp_pcb *pcb;
  int8_t closed_slot;
  int8_t err;
  union {
    struct {
      const char *data;
      size_t size;
      uint8_t apiflags;
    } write;
    size_t received;
    struct {
      ip_addr_t *addr;
      uint16_t port;
      tcp_connected_fn cb;
    } connect;
    struct {
      ip_addr_t *addr;
      uint16_t port;
    } bind;
    uint8_t backlog;
  };
} tcp_api_call_t;

static err_t _tcp_output_api(struct tcpip_api_call_data *api_call_msg) {
  tcp_api_call_t *msg = (tcp_api_call_t *)api_call_msg;
  msg->err = ERR_CONN;
  if (msg->closed_slot == INVALID_CLOSED_SLOT || !_closed_slots[msg->closed_slot]) {
    msg->err = tcp_output(msg->pcb);
  }
  return msg->err;
}

static esp_err_t _tcp_output(tcp_pcb *pcb, int8_t closed_slot) {
  if (!pcb) {
    return ERR_CONN;
  }
  tcp_api_call_t msg;
  msg.pcb = pcb;
  msg.closed_slot = closed_slot;
  tcpip_api_call(_tcp_output_api, (struct tcpip_api_call_data *)&msg);
  return msg.err;
}

static err_t _tcp_write_api(struct tcpip_api_call_data *api_call_msg) {
  tcp_api_call_t *msg = (tcp_api_call_t *)api_call_msg;
  msg->err = ERR_CONN;
  if (msg->closed_slot == INVALID_CLOSED_SLOT || !_closed_slots[msg->closed_slot]) {
    msg->err = tcp_write(msg->pcb, msg->write.data, msg->write.size, msg->write.apiflags);
  }
  return msg->err;
}

static esp_err_t _tcp_write(tcp_pcb *pcb, int8_t closed_slot, const char *data, size_t size, uint8_t apiflags) {
  if (!pcb) {
    return ERR_CONN;
  }
  tcp_api_call_t msg;
  msg.pcb = pcb;
  msg.closed_slot = closed_slot;
  msg.write.data = data;
  msg.write.size = size;
  msg.write.apiflags = apiflags;
  tcpip_api_call(_tcp_write_api, (struct tcpip_api_call_data *)&msg);
  return msg.err;
}

static err_t _tcp_recved_api(struct tcpip_api_call_data *api_call_msg) {
  tcp_api_call_t *msg = (tcp_api_call_t *)api_call_msg;
  msg->err = ERR_CONN;
  if (msg->closed_slot == INVALID_CLOSED_SLOT || !_closed_slots[msg->closed_slot]) {
    // if(msg->closed_slot != INVALID_CLOSED_SLOT && !_closed_slots[msg->closed_slot]) {
    // if(msg->closed_slot != INVALID_CLOSED_SLOT) {
    msg->err = 0;
    tcp_recved(msg->pcb, msg->received);
  }
  return msg->err;
}

static esp_err_t _tcp_recved(tcp_pcb *pcb, int8_t closed_slot, size_t len) {
  if (!pcb) {
    return ERR_CONN;
  }
  tcp_api_call_t msg;
  msg.pcb = pcb;
  msg.closed_slot = closed_slot;
  msg.received = len;
  tcpip_api_call(_tcp_recved_api, (struct tcpip_api_call_data *)&msg);
  return msg.err;
}

static err_t _tcp_close_api(struct tcpip_api_call_data *api_call_msg) {
  tcp_api_call_t *msg = (tcp_api_call_t *)api_call_msg;
  msg->err = ERR_CONN;
  if (msg->closed_slot == INVALID_CLOSED_SLOT || !_closed_slots[msg->closed_slot]) {
    msg->err = tcp_close(msg->pcb);
  }
  return msg->err;
}

static esp_err_t _tcp_close(tcp_pcb *pcb, int8_t closed_slot) {
  if (!pcb) {
    return ERR_CONN;
  }
  tcp_api_call_t msg;
  msg.pcb = pcb;
  msg.closed_slot = closed_slot;
  tcpip_api_call(_tcp_close_api, (struct tcpip_api_call_data *)&msg);
  return msg.err;
}

static err_t _tcp_abort_api(struct tcpip_api_call_data *api_call_msg) {
  tcp_api_call_t *msg = (tcp_api_call_t *)api_call_msg;
  msg->err = ERR_CONN;
  if (msg->closed_slot == INVALID_CLOSED_SLOT || !_closed_slots[msg->closed_slot]) {
    tcp_abort(msg->pcb);
  }
  return msg->err;
}

static esp_err_t _tcp_abort(tcp_pcb *pcb, int8_t closed_slot) {
  if (!pcb) {
    return ERR_CONN;
  }
  tcp_api_call_t msg;
  msg.pcb = pcb;
  msg.closed_slot = closed_slot;
  tcpip_api_call(_tcp_abort_api, (struct tcpip_api_call_data *)&msg);
  return msg.err;
}

static err_t _tcp_connect_api(struct tcpip_api_call_data *api_call_msg) {
  tcp_api_call_t *msg = (tcp_api_call_t *)api_call_msg;
  msg->err = tcp_connect(msg->pcb, msg->connect.addr, msg->connect.port, msg->connect.cb);
  return msg->err;
}

static esp_err_t _tcp_connect(tcp_pcb *pcb, int8_t closed_slot, ip_addr_t *addr, uint16_t port, tcp_connected_fn cb) {
  if (!pcb) {
    return ESP_FAIL;
  }
  tcp_api_call_t msg;
  msg.pcb = pcb;
  msg.closed_slot = closed_slot;
  msg.connect.addr = addr;
  msg.connect.port = port;
  msg.connect.cb = cb;
  tcpip_api_call(_tcp_connect_api, (struct tcpip_api_call_data *)&msg);
  return msg.err;
}

static err_t _tcp_bind_api(struct tcpip_api_call_data *api_call_msg) {
  tcp_api_call_t *msg = (tcp_api_call_t *)api_call_msg;
  msg->err = tcp_bind(msg->pcb, msg->bind.addr, msg->bind.port);
  return msg->err;
}

static esp_err_t _tcp_bind(tcp_pcb *pcb, ip_addr_t *addr, uint16_t port) {
  if (!pcb) {
    return ESP_FAIL;
  }
  tcp_api_call_t msg;
  msg.pcb = pcb;
  msg.closed_slot = -1;
  msg.bind.addr = addr;
  msg.bind.port = port;
  tcpip_api_call(_tcp_bind_api, (struct tcpip_api_call_data *)&msg);
  return msg.err;
}

static err_t _tcp_listen_api(struct tcpip_api_call_data *api_call_msg) {
  tcp_api_call_t *msg = (tcp_api_call_t *)api_call_msg;
  msg->err = 0;
  msg->pcb = tcp_listen_with_backlog(msg->pcb, msg->backlog);
  return msg->err;
}

static tcp_pcb *_tcp_listen_with_backlog(tcp_pcb *pcb, uint8_t backlog) {
  if (!pcb) {
    return NULL;
  }
  tcp_api_call_t msg;
  msg.pcb = pcb;
  msg.closed_slot = -1;
  msg.backlog = backlog ? backlog : 0xFF;
  tcpip_api_call(_tcp_listen_api, (struct tcpip_api_call_data *)&msg);
  return msg.pcb;
}

/*
  Async TCP Client
 */

AsyncClient::AsyncClient(tcp_pcb *pcb)
  : _connect_cb(0), _connect_cb_arg(0), _discard_cb(0), _discard_cb_arg(0), _sent_cb(0), _sent_cb_arg(0), _error_cb(0), _error_cb_arg(0), _recv_cb(0),
    _recv_cb_arg(0), _pb_cb(0), _pb_cb_arg(0), _timeout_cb(0), _timeout_cb_arg(0), _poll_cb(0), _poll_cb_arg(0), _ack_pcb(true), _tx_last_packet(0),
    _rx_timeout(0), _rx_last_ack(0), _ack_timeout(CONFIG_ASYNC_TCP_MAX_ACK_TIME), _connect_port(0) {
  _pcb = pcb;
  _closed_slot = INVALID_CLOSED_SLOT;
  if (_pcb) {
    _rx_last_packet = millis();
    tcp_arg(_pcb, this);
    tcp_recv(_pcb, &AsyncTCP_detail::tcp_recv);
    tcp_sent(_pcb, &AsyncTCP_detail::tcp_sent);
    tcp_err(_pcb, &AsyncTCP_detail::tcp_error);
    tcp_poll(_pcb, &AsyncTCP_detail::tcp_poll, CONFIG_ASYNC_TCP_POLL_TIMER);
    if (!_allocate_closed_slot()) {
      _close();
    }
  }
}

AsyncClient::~AsyncClient() {
  if (_pcb) {
    _close();
  }
  _free_closed_slot();
}

/*
 * Operators
 * */

bool AsyncClient::operator==(const AsyncClient &other) const {
  return _pcb == other._pcb;
}

/*
 * Callback Setters
 * */

void AsyncClient::onConnect(AcConnectHandler cb, void *arg) {
  _connect_cb = cb;
  _connect_cb_arg = arg;
}

void AsyncClient::onDisconnect(AcConnectHandler cb, void *arg) {
  _discard_cb = cb;
  _discard_cb_arg = arg;
}

void AsyncClient::onAck(AcAckHandler cb, void *arg) {
  _sent_cb = cb;
  _sent_cb_arg = arg;
}

void AsyncClient::onError(AcErrorHandler cb, void *arg) {
  _error_cb = cb;
  _error_cb_arg = arg;
}

void AsyncClient::onData(AcDataHandler cb, void *arg) {
  _recv_cb = cb;
  _recv_cb_arg = arg;
}

void AsyncClient::onPacket(AcPacketHandler cb, void *arg) {
  _pb_cb = cb;
  _pb_cb_arg = arg;
}

void AsyncClient::onTimeout(AcTimeoutHandler cb, void *arg) {
  _timeout_cb = cb;
  _timeout_cb_arg = arg;
}

void AsyncClient::onPoll(AcConnectHandler cb, void *arg) {
  _poll_cb = cb;
  _poll_cb_arg = arg;
}

/*
 * Main Public Methods
 * */

bool AsyncClient::connect(ip_addr_t addr, uint16_t port) {
  if (_pcb) {
    log_d("already connected, state %d", _pcb->state);
    return false;
  }
  if (!_start_async_task()) {
    log_e("failed to start task");
    return false;
  }

  if (!_allocate_closed_slot()) {
    log_e("failed to allocate: closed slot full");
    return false;
  }

  tcp_pcb *pcb;
  {
    tcp_core_guard tcg;
    pcb = tcp_new_ip_type(addr.type);
    if (!pcb) {
      log_e("pcb == NULL");
      return false;
    }
    tcp_arg(pcb, this);
    tcp_err(pcb, &AsyncTCP_detail::tcp_error);
    tcp_recv(pcb, &AsyncTCP_detail::tcp_recv);
    tcp_sent(pcb, &AsyncTCP_detail::tcp_sent);
    tcp_poll(pcb, &AsyncTCP_detail::tcp_poll, CONFIG_ASYNC_TCP_POLL_TIMER);
  }

  esp_err_t err = _tcp_connect(pcb, _closed_slot, &addr, port, (tcp_connected_fn)&_tcp_connected);
  return err == ESP_OK;
}

#ifdef ARDUINO
bool AsyncClient::connect(const IPAddress &ip, uint16_t port) {
  ip_addr_t addr;
#if ESP_IDF_VERSION_MAJOR < 5
  addr.u_addr.ip4.addr = ip;
  addr.type = IPADDR_TYPE_V4;
#else
  ip.to_ip_addr_t(&addr);
#endif

  return connect(addr, port);
}
#endif

#if LWIP_IPV6 && ESP_IDF_VERSION_MAJOR < 5
bool AsyncClient::connect(const IPv6Address &ip, uint16_t port) {
  auto ipaddr = static_cast<const uint32_t *>(ip);
  ip_addr_t addr = IPADDR6_INIT(ipaddr[0], ipaddr[1], ipaddr[2], ipaddr[3]);

  return connect(addr, port);
}
#endif

bool AsyncClient::connect(const char *host, uint16_t port) {
  ip_addr_t addr;

  if (!_start_async_task()) {
    log_e("failed to start task");
    return false;
  }

  err_t err;
  {
    tcp_core_guard tcg;
    err = dns_gethostbyname(host, &addr, (dns_found_callback)&_tcp_dns_found, this);
  }

  if (err == ERR_OK) {
#if ESP_IDF_VERSION_MAJOR < 5
#if LWIP_IPV6
    if (addr.type == IPADDR_TYPE_V6) {
      return connect(IPv6Address(addr.u_addr.ip6.addr), port);
    }
    return connect(IPAddress(addr.u_addr.ip4.addr), port);
#else
    return connect(IPAddress(addr.addr), port);
#endif
#else
    return connect(addr, port);
#endif
  } else if (err == ERR_INPROGRESS) {
    _connect_port = port;
    return true;
  }
  log_d("error: %d", err);
  return false;
}

void AsyncClient::close(bool now) {
  if (_pcb) {
    _tcp_recved(_pcb, _closed_slot, _rx_ack_len);
  }
  _close();
}

int8_t AsyncClient::abort() {
  if (_pcb) {
    _tcp_abort(_pcb, _closed_slot);
    _pcb = NULL;
  }
  return ERR_ABRT;
}

size_t AsyncClient::space() const {
  if ((_pcb != NULL) && (_pcb->state == ESTABLISHED)) {
    return tcp_sndbuf(_pcb);
  }
  return 0;
}

size_t AsyncClient::add(const char *data, size_t size, uint8_t apiflags) {
  if (!_pcb || size == 0 || data == NULL) {
    return 0;
  }
  size_t room = space();
  if (!room) {
    return 0;
  }
  size_t will_send = (room < size) ? room : size;
  int8_t err = ERR_OK;
  err = _tcp_write(_pcb, _closed_slot, data, will_send, apiflags);
  if (err != ERR_OK) {
    return 0;
  }
  return will_send;
}

bool AsyncClient::send() {
  auto backup = _tx_last_packet;
  _tx_last_packet = millis();
  if (_tcp_output(_pcb, _closed_slot) == ERR_OK) {
    return true;
  }
  _tx_last_packet = backup;
  return false;
}

size_t AsyncClient::ack(size_t len) {
  if (len > _rx_ack_len) {
    len = _rx_ack_len;
  }
  if (len) {
    _tcp_recved(_pcb, _closed_slot, len);
  }
  _rx_ack_len -= len;
  return len;
}

void AsyncClient::ackPacket(struct pbuf *pb) {
  if (!pb) {
    return;
  }
  _tcp_recved(_pcb, _closed_slot, pb->len);
  pbuf_free(pb);
}

/*
 * Main Private Methods
 * */

int8_t AsyncClient::_close() {
  // ets_printf("X: 0x%08x\n", (uint32_t)this);
  int8_t err = ERR_OK;
  if (_pcb) {
    {
      tcp_core_guard tcg;
      tcp_arg(_pcb, NULL);
      tcp_sent(_pcb, NULL);
      tcp_recv(_pcb, NULL);
      tcp_err(_pcb, NULL);
      tcp_poll(_pcb, NULL, 0);
    }
    _tcp_clear_events(this);
    err = _tcp_close(_pcb, _closed_slot);
    if (err != ERR_OK) {
      err = abort();
    }
    _free_closed_slot();
    _pcb = NULL;
    if (_discard_cb) {
      _discard_cb(_discard_cb_arg, this);
    }
  }
  return err;
}

bool AsyncClient::_allocate_closed_slot() {
  bool allocated = false;
  if (xSemaphoreTake(_slots_lock, portMAX_DELAY) == pdTRUE) {
    uint32_t closed_slot_min_index = 0;
    allocated = _closed_slot != INVALID_CLOSED_SLOT;
    if (!allocated) {
      for (int i = 0; i < _number_of_closed_slots; ++i) {
        if ((_closed_slot == INVALID_CLOSED_SLOT || _closed_slots[i] <= closed_slot_min_index) && _closed_slots[i] != 0) {
          closed_slot_min_index = _closed_slots[i];
          _closed_slot = i;
        }
      }
      allocated = _closed_slot != INVALID_CLOSED_SLOT;
      if (allocated) {
        _closed_slots[_closed_slot] = 0;
      }
    }
    xSemaphoreGive(_slots_lock);
  }
  return allocated;
}

void AsyncClient::_free_closed_slot() {
  xSemaphoreTake(_slots_lock, portMAX_DELAY);
  if (_closed_slot != INVALID_CLOSED_SLOT) {
    _closed_slots[_closed_slot] = _closed_index;
    _closed_slot = INVALID_CLOSED_SLOT;
    ++_closed_index;
  }
  xSemaphoreGive(_slots_lock);
}

/*
 * Private Callbacks
 * */

int8_t AsyncClient::_connected(tcp_pcb *pcb, int8_t err) {
  _pcb = reinterpret_cast<tcp_pcb *>(pcb);
  if (_pcb) {
    _rx_last_packet = millis();
  }
  _tx_last_packet = 0;
  _rx_last_ack = 0;
  if (_connect_cb) {
    _connect_cb(_connect_cb_arg, this);
  }
  return ERR_OK;
}

void AsyncClient::_error(int8_t err) {
  if (_error_cb) {
    _error_cb(_error_cb_arg, this, err);
  }
  if (_discard_cb) {
    _discard_cb(_discard_cb_arg, this);
  }
}

// In LwIP Thread
int8_t AsyncClient::_lwip_fin(tcp_pcb *pcb, int8_t err) {
  if (!_pcb || pcb != _pcb) {
    log_d("0x%08" PRIx32 " != 0x%08" PRIx32, (uint32_t)pcb, (uint32_t)_pcb);
    return ERR_OK;
  }
  tcp_arg(_pcb, NULL);
  if (_pcb->state == LISTEN) {
    tcp_sent(_pcb, NULL);
    tcp_recv(_pcb, NULL);
    tcp_err(_pcb, NULL);
    tcp_poll(_pcb, NULL, 0);
  }
  if (tcp_close(_pcb) != ERR_OK) {
    tcp_abort(_pcb);
  }
  _free_closed_slot();
  _pcb = NULL;
  return ERR_OK;
}

// In Async Thread
int8_t AsyncClient::_fin(tcp_pcb *pcb, int8_t err) {
  _tcp_clear_events(this);
  if (_discard_cb) {
    _discard_cb(_discard_cb_arg, this);
  }
  return ERR_OK;
}

int8_t AsyncClient::_sent(tcp_pcb *pcb, uint16_t len) {
  _rx_last_ack = _rx_last_packet = millis();
  if (_sent_cb) {
    _sent_cb(_sent_cb_arg, this, len, (_rx_last_packet - _tx_last_packet));
  }
  return ERR_OK;
}

int8_t AsyncClient::_recv(tcp_pcb *pcb, pbuf *pb, int8_t err) {
  while (pb != NULL) {
    _rx_last_packet = millis();
    // we should not ack before we assimilate the data
    _ack_pcb = true;
    pbuf *b = pb;
    pb = b->next;
    b->next = NULL;
    if (_pb_cb) {
      _pb_cb(_pb_cb_arg, this, b);
    } else {
      if (_recv_cb) {
        _recv_cb(_recv_cb_arg, this, b->payload, b->len);
      }
      if (!_ack_pcb) {
        _rx_ack_len += b->len;
      } else if (_pcb) {
        _tcp_recved(_pcb, _closed_slot, b->len);
      }
    }
    pbuf_free(b);
  }
  return ERR_OK;
}

int8_t AsyncClient::_poll(tcp_pcb *pcb) {
  if (!_pcb) {
    // log_d("pcb is NULL");
    return ERR_OK;
  }
  if (pcb != _pcb) {
    log_d("0x%08" PRIx32 " != 0x%08" PRIx32, (uint32_t)pcb, (uint32_t)_pcb);
    return ERR_OK;
  }

  uint32_t now = millis();

  // ACK Timeout
  if (_ack_timeout) {
    const uint32_t one_day = 86400000;
    bool last_tx_is_after_last_ack = (_rx_last_ack - _tx_last_packet + one_day) < one_day;
    if (last_tx_is_after_last_ack && (now - _tx_last_packet) >= _ack_timeout) {
      log_d("ack timeout %d", pcb->state);
      if (_timeout_cb) {
        _timeout_cb(_timeout_cb_arg, this, (now - _tx_last_packet));
      }
      return ERR_OK;
    }
  }
  // RX Timeout
  if (_rx_timeout && (now - _rx_last_packet) >= (_rx_timeout * 1000)) {
    log_d("rx timeout %d", pcb->state);
    _close();
    return ERR_OK;
  }
  // Everything is fine
  if (_poll_cb) {
    _poll_cb(_poll_cb_arg, this);
  }
  return ERR_OK;
}

void AsyncClient::_dns_found(struct ip_addr *ipaddr) {
  if (ipaddr) {
    connect(*ipaddr, _connect_port);
  } else {
    if (_error_cb) {
      _error_cb(_error_cb_arg, this, -55);
    }
    if (_discard_cb) {
      _discard_cb(_discard_cb_arg, this);
    }
  }
}

/*
 * Public Helper Methods
 * */

bool AsyncClient::free() {
  if (!_pcb) {
    return true;
  }
  if (_pcb->state == CLOSED || _pcb->state > ESTABLISHED) {
    return true;
  }
  return false;
}

size_t AsyncClient::write(const char *data, size_t size, uint8_t apiflags) {
  size_t will_send = add(data, size, apiflags);
  if (!will_send || !send()) {
    return 0;
  }
  return will_send;
}

void AsyncClient::setRxTimeout(uint32_t timeout) {
  _rx_timeout = timeout;
}

uint32_t AsyncClient::getRxTimeout() const {
  return _rx_timeout;
}

uint32_t AsyncClient::getAckTimeout() const {
  return _ack_timeout;
}

void AsyncClient::setAckTimeout(uint32_t timeout) {
  _ack_timeout = timeout;
}

void AsyncClient::setNoDelay(bool nodelay) const {
  if (!_pcb) {
    return;
  }
  if (nodelay) {
    tcp_nagle_disable(_pcb);
  } else {
    tcp_nagle_enable(_pcb);
  }
}

bool AsyncClient::getNoDelay() {
  if (!_pcb) {
    return false;
  }
  return tcp_nagle_disabled(_pcb);
}

void AsyncClient::setKeepAlive(uint32_t ms, uint8_t cnt) {
  if (ms != 0) {
    _pcb->so_options |= SOF_KEEPALIVE;  // Turn on TCP Keepalive for the given pcb
    // Set the time between keepalive messages in milli-seconds
    _pcb->keep_idle = ms;
    _pcb->keep_intvl = ms;
    _pcb->keep_cnt = cnt;  // The number of unanswered probes required to force closure of the socket
  } else {
    _pcb->so_options &= ~SOF_KEEPALIVE;  // Turn off TCP Keepalive for the given pcb
  }
}

uint16_t AsyncClient::getMss() const {
  if (!_pcb) {
    return 0;
  }
  return tcp_mss(_pcb);
}

uint32_t AsyncClient::getRemoteAddress() const {
  if (!_pcb) {
    return 0;
  }
#if LWIP_IPV4 && LWIP_IPV6
  return _pcb->remote_ip.u_addr.ip4.addr;
#else
  return _pcb->remote_ip.addr;
#endif
}

#if LWIP_IPV6
ip6_addr_t AsyncClient::getRemoteAddress6() const {
  if (_pcb && _pcb->remote_ip.type == IPADDR_TYPE_V6) {
    return _pcb->remote_ip.u_addr.ip6;
  } else {
    ip6_addr_t nulladdr;
    ip6_addr_set_zero(&nulladdr);
    return nulladdr;
  }
}

ip6_addr_t AsyncClient::getLocalAddress6() const {
  if (_pcb && _pcb->local_ip.type == IPADDR_TYPE_V6) {
    return _pcb->local_ip.u_addr.ip6;
  } else {
    ip6_addr_t nulladdr;
    ip6_addr_set_zero(&nulladdr);
    return nulladdr;
  }
}
#ifdef ARDUINO
#if ESP_IDF_VERSION_MAJOR < 5
IPv6Address AsyncClient::remoteIP6() const {
  return IPv6Address(getRemoteAddress6().addr);
}

IPv6Address AsyncClient::localIP6() const {
  return IPv6Address(getLocalAddress6().addr);
}
#else
IPAddress AsyncClient::remoteIP6() const {
  if (!_pcb) {
    return IPAddress(IPType::IPv6);
  }
  IPAddress ip;
  ip.from_ip_addr_t(&(_pcb->remote_ip));
  return ip;
}

IPAddress AsyncClient::localIP6() const {
  if (!_pcb) {
    return IPAddress(IPType::IPv6);
  }
  IPAddress ip;
  ip.from_ip_addr_t(&(_pcb->local_ip));
  return ip;
}
#endif
#endif
#endif

uint16_t AsyncClient::getRemotePort() const {
  if (!_pcb) {
    return 0;
  }
  return _pcb->remote_port;
}

uint32_t AsyncClient::getLocalAddress() const {
  if (!_pcb) {
    return 0;
  }
#if LWIP_IPV4 && LWIP_IPV6
  return _pcb->local_ip.u_addr.ip4.addr;
#else
  return _pcb->local_ip.addr;
#endif
}

uint16_t AsyncClient::getLocalPort() const {
  if (!_pcb) {
    return 0;
  }
  return _pcb->local_port;
}

ip4_addr_t AsyncClient::getRemoteAddress4() const {
  if (_pcb && _pcb->remote_ip.type == IPADDR_TYPE_V4) {
    return _pcb->remote_ip.u_addr.ip4;
  } else {
    ip4_addr_t nulladdr;
    ip4_addr_set_zero(&nulladdr);
    return nulladdr;
  }
}

ip4_addr_t AsyncClient::getLocalAddress4() const {
  if (_pcb && _pcb->local_ip.type == IPADDR_TYPE_V4) {
    return _pcb->local_ip.u_addr.ip4;
  } else {
    ip4_addr_t nulladdr;
    ip4_addr_set_zero(&nulladdr);
    return nulladdr;
  }
}

#ifdef ARDUINO
IPAddress AsyncClient::remoteIP() const {
#if ESP_IDF_VERSION_MAJOR < 5
  return IPAddress(getRemoteAddress());
#else
  if (!_pcb) {
    return IPAddress();
  }
  IPAddress ip;
  ip.from_ip_addr_t(&(_pcb->remote_ip));
  return ip;
#endif
}

IPAddress AsyncClient::localIP() const {
#if ESP_IDF_VERSION_MAJOR < 5
  return IPAddress(getLocalAddress());
#else
  if (!_pcb) {
    return IPAddress();
  }
  IPAddress ip;
  ip.from_ip_addr_t(&(_pcb->local_ip));
  return ip;
#endif
}
#endif

uint8_t AsyncClient::state() const {
  if (!_pcb) {
    return 0;
  }
  return _pcb->state;
}

bool AsyncClient::connected() const {
  if (!_pcb) {
    return false;
  }
  return _pcb->state == ESTABLISHED;
}

bool AsyncClient::connecting() const {
  if (!_pcb) {
    return false;
  }
  return _pcb->state > CLOSED && _pcb->state < ESTABLISHED;
}

bool AsyncClient::disconnecting() const {
  if (!_pcb) {
    return false;
  }
  return _pcb->state > ESTABLISHED && _pcb->state < TIME_WAIT;
}

bool AsyncClient::disconnected() const {
  if (!_pcb) {
    return true;
  }
  return _pcb->state == CLOSED || _pcb->state == TIME_WAIT;
}

bool AsyncClient::freeable() const {
  if (!_pcb) {
    return true;
  }
  return _pcb->state == CLOSED || _pcb->state > ESTABLISHED;
}

bool AsyncClient::canSend() const {
  return space() > 0;
}

const char *AsyncClient::errorToString(int8_t error) {
  switch (error) {
    case ERR_OK:         return "OK";
    case ERR_MEM:        return "Out of memory error";
    case ERR_BUF:        return "Buffer error";
    case ERR_TIMEOUT:    return "Timeout";
    case ERR_RTE:        return "Routing problem";
    case ERR_INPROGRESS: return "Operation in progress";
    case ERR_VAL:        return "Illegal value";
    case ERR_WOULDBLOCK: return "Operation would block";
    case ERR_USE:        return "Address in use";
    case ERR_ALREADY:    return "Already connected";
    case ERR_CONN:       return "Not connected";
    case ERR_IF:         return "Low-level netif error";
    case ERR_ABRT:       return "Connection aborted";
    case ERR_RST:        return "Connection reset";
    case ERR_CLSD:       return "Connection closed";
    case ERR_ARG:        return "Illegal argument";
    case -55:            return "DNS failed";
    default:             return "UNKNOWN";
  }
}

const char *AsyncClient::stateToString() const {
  switch (state()) {
    case 0:  return "Closed";
    case 1:  return "Listen";
    case 2:  return "SYN Sent";
    case 3:  return "SYN Received";
    case 4:  return "Established";
    case 5:  return "FIN Wait 1";
    case 6:  return "FIN Wait 2";
    case 7:  return "Close Wait";
    case 8:  return "Closing";
    case 9:  return "Last ACK";
    case 10: return "Time Wait";
    default: return "UNKNOWN";
  }
}

/*
  Async TCP Server
 */

AsyncServer::AsyncServer(ip_addr_t addr, uint16_t port)
  : _port(port), _addr(addr), _noDelay(false), _pcb(nullptr), _connect_cb(nullptr), _connect_cb_arg(nullptr) {}

#ifdef ARDUINO
AsyncServer::AsyncServer(IPAddress addr, uint16_t port) : _port(port), _noDelay(false), _pcb(0), _connect_cb(0), _connect_cb_arg(0) {
#if ESP_IDF_VERSION_MAJOR < 5
  _addr.type = IPADDR_TYPE_V4;
  _addr.u_addr.ip4.addr = addr;
#else
  addr.to_ip_addr_t(&_addr);
#endif
}
#if ESP_IDF_VERSION_MAJOR < 5
AsyncServer::AsyncServer(IPv6Address addr, uint16_t port) : _port(port), _noDelay(false), _pcb(0), _connect_cb(0), _connect_cb_arg(0) {
  _addr.type = IPADDR_TYPE_V6;
  auto ipaddr = static_cast<const uint32_t *>(addr);
  _addr = IPADDR6_INIT(ipaddr[0], ipaddr[1], ipaddr[2], ipaddr[3]);
}
#endif
#endif

AsyncServer::AsyncServer(uint16_t port) : _port(port), _noDelay(false), _pcb(0), _connect_cb(0), _connect_cb_arg(0) {
  _addr.type = IPADDR_TYPE_V4;
  _addr.u_addr.ip4.addr = INADDR_ANY;
}

AsyncServer::~AsyncServer() {
  end();
}

void AsyncServer::onClient(AcConnectHandler cb, void *arg) {
  _connect_cb = cb;
  _connect_cb_arg = arg;
}

void AsyncServer::begin() {
  if (_pcb) {
    return;
  }

  if (!_start_async_task()) {
    log_e("failed to start task");
    return;
  }
  int8_t err;
  {
    tcp_core_guard tcg;
    _pcb = tcp_new_ip_type(_addr.type);
  }
  if (!_pcb) {
    log_e("_pcb == NULL");
    return;
  }

  err = _tcp_bind(_pcb, &_addr, _port);

  if (err != ERR_OK) {
    _tcp_close(_pcb, -1);
    _pcb = NULL;
    log_e("bind error: %d", err);
    return;
  }

  static uint8_t backlog = 5;
  _pcb = _tcp_listen_with_backlog(_pcb, backlog);
  if (!_pcb) {
    log_e("listen_pcb == NULL");
    return;
  }
  tcp_core_guard tcg;
  tcp_arg(_pcb, (void *)this);
  tcp_accept(_pcb, &AsyncTCP_detail::tcp_accept);
}

void AsyncServer::end() {
  if (_pcb) {
    tcp_core_guard tcg;
    tcp_arg(_pcb, NULL);
    tcp_accept(_pcb, NULL);
    if (tcp_close(_pcb) != ERR_OK) {
      tcp_abort(_pcb);
    }
    _pcb = NULL;
  }
}

// runs on LwIP thread
int8_t AsyncTCP_detail::tcp_accept(void *arg, tcp_pcb *pcb, int8_t err) {
  if (!pcb) {
    log_e("_accept failed: pcb is NULL");
    return ERR_ABRT;
  }
  auto server = reinterpret_cast<AsyncServer *>(arg);
  if (server->_connect_cb) {
    AsyncClient *c = new (std::nothrow) AsyncClient(pcb);
    if (c && c->pcb()) {
      c->setNoDelay(server->_noDelay);

      lwip_tcp_event_packet_t *e = (lwip_tcp_event_packet_t *)malloc(sizeof(lwip_tcp_event_packet_t));
      if (e) {
        e->event = LWIP_TCP_ACCEPT;
        e->accept.server = server;
        e->client = c;
        if (_prepend_async_event(&e)) {
          return ERR_OK;  // success
        }
        free((void *)(e));
      }

      // Couldn't allocate accept event
      // We can't let the client object call in to close, as we're on the LWIP thread; it could deadlock trying to RPC to itself
      c->_pcb = nullptr;
      tcp_abort(pcb);
      log_e("_accept failed: couldn't accept client");
      return ERR_ABRT;
    }
    if (c) {
      // Couldn't complete setup
      // pcb has already been aborted
      delete c;
      pcb = nullptr;
      log_e("_accept failed: couldn't complete setup");
      return ERR_ABRT;
    }
    log_e("_accept failed: couldn't allocate client");
  } else {
    log_e("_accept failed: no onConnect callback");
  }
  tcp_abort(pcb);
  return ERR_OK;
}

int8_t AsyncServer::_accepted(AsyncClient *client) {
  if (_connect_cb) {
    _connect_cb(_connect_cb_arg, client);
  }
  return ERR_OK;
}

void AsyncServer::setNoDelay(bool nodelay) {
  _noDelay = nodelay;
}

bool AsyncServer::getNoDelay() const {
  return _noDelay;
}

uint8_t AsyncServer::status() const {
  if (!_pcb) {
    return 0;
  }
  return _pcb->state;
}
