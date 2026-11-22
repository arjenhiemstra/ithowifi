/*
   Servicetool bridge - implementation
   See ServicetoolBridge.h for the overall behaviour and
   ServicetoolProtocol.h for the wire format.

   Threading:
     - TaskServicetool owns both sockets (listen + client). Only this task
       reads from or writes to them. It is created in boot() and gates on
       PHASE_CONFIG | PHASE_NET (ADR-0010).
     - The client loop is non-blocking: pings are answered immediately,
       frames are queued and executed one at a time as items on the existing
       I2C command queue (run by TaskSysControl), so they are serialised with
       all other I2C traffic. Each item holds the I2C bus lock across send +
       receive. The loop picks up the finished item and forwards the reply.
     - Session state is exposed through volatile flags + a mutex-protected
       info struct; MQTT publishing happens in the MQTT task.

   Part of NRG itho wifi firmware
*/

#include "managers/ServicetoolBridge.h"
#include "managers/ServicetoolProtocol.h"

#include <memory>
#include <deque>
#include <vector>
#include <cerrno>
#include <cstring>
#include <ctime>
#include <unistd.h>

#include "lwip/sockets.h"
#include "lwip/inet.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "globals.h"
#include "sys_log.h"
#include "config/SystemConfig.h"
#include "ithodevice/i2c_esp32.h"
#include "ithodevice/IthoDevice.h"
#include "tasks/task_mqtt.h"
#include "tasks/boot_phases.h"

using servicetool::DleDecoder;

#define TASK_SERVICETOOL_PRIO 3
#define SERVICETOOL_OWN_ADDRESS (I2C_SLAVE_ADDRESS << 1) // 0x80, our address on the Itho bus
#define SERVICETOOL_QUEUE_EXTRA_WAIT_MS 2500 // time a frame may wait in the I2C queue
#define SERVICETOOL_MQTT_REFRESH_MS 60000UL
#define SERVICETOOL_BLOCK_LOG_LIMIT 5
#define SERVICETOOL_MAX_PENDING 16

// ---------------------------------------------------------------------------
// shared state
// ---------------------------------------------------------------------------

volatile bool servicetoolSessionActive = false;
volatile bool servicetoolMqttUpdate = true; // publish "inactive" after boot/connect
volatile bool servicetoolResumePolling = false;
volatile bool servicetoolEndRequested = false;

namespace
{
  struct SessionInfo
  {
    char client[16]{};
    time_t since{0};
    time_t lastActivity{0};
    uint32_t framesTx{0};
    uint32_t framesRx{0};
    uint32_t framesRejected{0};
  };

  SessionInfo info;
  SemaphoreHandle_t infoMutex = nullptr;
  volatile uint32_t blockedCount = 0;

  TaskHandle_t taskHandle = nullptr;

  struct BridgeRequest
  {
    uint8_t tx[servicetool::MAX_FRAME]{};
    size_t txLen{0};
    uint8_t rx[I2C_SLAVE_RX_BUF_LEN]{};
    size_t rxLen{0};
    bool sent{false};
    uint32_t windowMs{50};
    unsigned long queuedAt{0};
    volatile bool abort{false};
    SemaphoreHandle_t done{nullptr};

    BridgeRequest() { done = xSemaphoreCreateBinary(); }
    ~BridgeRequest()
    {
      if (done)
        vSemaphoreDelete(done);
    }
  };

  time_t nowEpoch()
  {
    time_t now;
    time(&now);
    return now;
  }

  void lockInfo()
  {
    if (infoMutex)
      xSemaphoreTake(infoMutex, portMAX_DELAY);
  }
  void unlockInfo()
  {
    if (infoMutex)
      xSemaphoreGive(infoMutex);
  }

  void closeFd(int &fd)
  {
    if (fd >= 0)
    {
      shutdown(fd, SHUT_RDWR);
      close(fd);
      fd = -1;
    }
  }

  bool sendAll(int fd, const uint8_t *buf, size_t len)
  {
    size_t off = 0;
    while (off < len)
    {
      int n = send(fd, buf + off, len - off, 0);
      if (n <= 0)
        return false;
      off += static_cast<size_t>(n);
    }
    return true;
  }

  int openListenSocket(uint16_t port)
  {
    int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd < 0)
      return -1;

    int yes = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) != 0 || listen(fd, 1) != 0)
    {
      close(fd);
      return -1;
    }
    return fd;
  }

  void configureClientSocket(int fd)
  {
    int yes = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes));

    // detect half-open connections (laptop lid closed, WiFi gone)
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(yes));
    int idle = 30, intvl = 10, cnt = 3;
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(intvl));
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &cnt, sizeof(cnt));

    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
  }

  // Wait until fd is readable or timeoutMs passes. >0 readable, 0 timeout, <0 error.
  int waitReadable(int fd, uint32_t timeoutMs)
  {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    struct timeval tv;
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;
    return select(fd + 1, &rfds, nullptr, nullptr, &tv);
  }

  void startSession(const char *client)
  {
    lockInfo();
    info = SessionInfo();
    strlcpy(info.client, client, sizeof(info.client));
    info.since = nowEpoch();
    info.lastActivity = info.since;
    unlockInfo();
    blockedCount = 0;

    servicetoolSessionActive = true;
    servicetoolMqttUpdate = true;
    sysStatReq = true;
    N_LOG("ST: servicetool session started from %s - I2C polling paused, commands blocked", client);
  }

  void endSession(const char *reason)
  {
    if (!servicetoolSessionActive)
      return;

    servicetoolSessionActive = false;
    servicetoolResumePolling = true;
    servicetoolMqttUpdate = true;
    sysStatReq = true;

    lockInfo();
    uint32_t tx = info.framesTx, rx = info.framesRx, rej = info.framesRejected;
    unlockInfo();
    N_LOG("ST: servicetool session ended (%s) - frames tx:%u rx:%u rejected:%u, blocked commands:%u",
          reason, tx, rx, rej, static_cast<unsigned>(blockedCount));
  }

  void touchActivity()
  {
    lockInfo();
    info.lastActivity = nowEpoch();
    unlockInfo();
  }

  void countRejected()
  {
    lockInfo();
    info.framesRejected++;
    unlockInfo();
  }

  // Check a frame from the servicetool before it goes on the bus.
  bool acceptFrame(const uint8_t *frame, size_t len)
  {
    if (!servicetool::frameValid(frame, len))
    {
      // Short non-I2C frames (e.g. 00 00 00) are not meant for the bus. In
      // practice they appear when a ping was answered too late.
      countRejected();
      if (len == 3)
        W_LOG("ST: ignored 3-byte interface command 0x%02X (late ping?)", frame[2]);
      else
        W_LOG("ST: rejected malformed frame from servicetool (len:%u)", static_cast<unsigned>(len));
      return false;
    }
    uint16_t msgClass = servicetool::frameClass(frame);
    if (servicetool::classBlocked(msgClass))
    {
      countRejected();
      W_LOG("ST: blocked firmware/bootloader frame class 0x%04X (flashing over WiFi is disabled)", msgClass);
      return false;
    }
    return true;
  }

  // Put one frame on the I2C queue. Returns nullptr if it could not be queued.
  std::shared_ptr<BridgeRequest> dispatchFrame(const std::vector<uint8_t> &frame)
  {
    auto req = std::make_shared<BridgeRequest>();
    if (!req->done)
      return nullptr;
    memcpy(req->tx, frame.data(), frame.size());
    req->txLen = frame.size();
    req->windowMs = servicetool::replyWindowMs(frame.data());
    req->queuedAt = millis();

    if (i2cManager.queueSize() >= I2C_CMD_QUEUE_MAX_SIZE)
    {
      W_LOG("ST: I2C queue full, frame dropped");
      return nullptr;
    }

    D_LOG("ST: tx %s", i2cBufToString(req->tx, req->txLen).c_str());

    i2cQueueAddCmd([req]()
                   {
                     // hold the bus across send + receive so no other task can
                     // use it between the servicetool's question and the reply
                     I2CBusLock lock;
                     req->sent = i2cSendBytes(req->tx, req->txLen, I2C_CMD_SERVICETOOL);
                     if (req->sent)
                       req->rxLen = i2cSlaveReceive(req->rx, req->windowMs, &req->abort);
                     xSemaphoreGive(req->done); });
    return req;
  }

  // Forward whatever the bus returned for a finished request.
  // Returns false if the TCP connection broke.
  bool finishRequest(int clientFd, const std::shared_ptr<BridgeRequest> &req)
  {
    uint16_t msgClass = servicetool::frameClass(req->tx);
    lockInfo();
    if (req->sent)
      info.framesTx++;
    unlockInfo();

    if (!req->sent)
    {
      // e.g. no node at that address (the servicetool probes 0x80, 0x60..0x66, 0x20)
      D_LOG("ST: no ACK on bus for class 0x%04X to 0x%02X", msgClass, req->tx[0]);
      return true;
    }
    if (req->rxLen == 0)
    {
      D_LOG("ST: no reply for class 0x%04X%s", msgClass, req->abort ? " (superseded by next frame)" : "");
      return true;
    }

    D_LOG("ST: rx %s", i2cBufToString(req->rx, req->rxLen).c_str());

    bool ok = true;
    static uint8_t out[servicetool::MAX_FRAME * 2 + 4];
    size_t frames = servicetool::splitReceived(
        req->rx, req->rxLen, SERVICETOOL_OWN_ADDRESS,
        [&](const uint8_t *f, size_t flen)
        {
          size_t n = servicetool::dleEncode(f, flen, out, sizeof(out));
          if (n && ok)
            ok = sendAll(clientFd, out, n);
        });

    if (frames == 0)
      W_LOG("ST: could not parse reply (%u bytes)", static_cast<unsigned>(req->rxLen));

    lockInfo();
    info.framesRx += frames;
    unlockInfo();
    return ok;
  }

  // Serve one connected client until it disconnects or the session ends.
  //
  // Event loop: the socket is read continuously, so pings are answered
  // immediately even while an I2C request is in flight. Frames are executed
  // one at a time; a new frame from the servicetool means it stopped waiting
  // for the previous reply, so the in-flight receive is aborted to free the
  // bus straight away (otherwise unanswered queries pile up, replies get
  // later and later, and pings time out -> the servicetool stops working).
  void serveClient(int clientFd)
  {
    DleDecoder decoder;
    uint8_t buf[256];
    std::deque<std::vector<uint8_t>> pending;
    std::shared_ptr<BridgeRequest> inflight;
    unsigned long lastActivityMs = millis();
    unsigned long lastMqttRefresh = millis();
    const char *reason = "client disconnected";

    for (;;)
    {
      if (servicetoolEndRequested)
      {
        servicetoolEndRequested = false;
        reason = "ended from web interface";
        break;
      }
      if (systemConfig.servicetool_bridge != 1)
      {
        reason = "bridge disabled";
        break;
      }
      uint32_t timeoutMs = static_cast<uint32_t>(systemConfig.servicetool_timeout) * 60000UL;
      if (timeoutMs > 0 && millis() - lastActivityMs > timeoutMs)
      {
        reason = "inactivity timeout";
        break;
      }
      if (millis() - lastMqttRefresh > SERVICETOOL_MQTT_REFRESH_MS)
      {
        lastMqttRefresh = millis();
        servicetoolMqttUpdate = true;
      }

      // 1. finished request -> forward reply
      if (inflight)
      {
        if (xSemaphoreTake(inflight->done, 0) == pdTRUE)
        {
          bool ok = finishRequest(clientFd, inflight);
          inflight.reset();
          if (!ok)
          {
            reason = "send to client failed";
            break;
          }
        }
        else if (millis() - inflight->queuedAt > inflight->windowMs + SERVICETOOL_QUEUE_EXTRA_WAIT_MS)
        {
          W_LOG("ST: I2C queue did not execute frame in time, skipped");
          inflight->abort = true;
          inflight.reset(); // the queued lambda keeps its own reference
        }
      }

      // 2. next frame -> I2C queue
      while (!inflight && !pending.empty())
      {
        inflight = dispatchFrame(pending.front());
        pending.pop_front();
      }

      // 3. read from the servicetool
      int r = waitReadable(clientFd, (inflight || !pending.empty()) ? 10 : 200);
      if (r < 0)
      {
        reason = "socket error";
        break;
      }
      if (r == 0)
        continue;

      int n = recv(clientFd, buf, sizeof(buf), 0);
      if (n == 0)
        break; // orderly close
      if (n < 0)
      {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
          continue;
        reason = "connection lost";
        break;
      }

      lastActivityMs = millis();
      touchActivity();

      bool alive = true;
      for (int i = 0; i < n && alive; i++)
      {
        switch (decoder.feed(buf[i]))
        {
        case DleDecoder::Event::Ping:
        {
          const uint8_t pong[2] = {servicetool::DLE, servicetool::SYN};
          alive = sendAll(clientFd, pong, sizeof(pong));
          break;
        }
        case DleDecoder::Event::Frame:
          if (acceptFrame(decoder.data(), decoder.size()))
          {
            if (inflight)
              inflight->abort = true; // servicetool moved on
            if (pending.size() >= SERVICETOOL_MAX_PENDING)
            {
              W_LOG("ST: too many pending frames, oldest dropped");
              pending.pop_front();
            }
            pending.emplace_back(decoder.data(), decoder.data() + decoder.size());
          }
          break;
        case DleDecoder::Event::Overflow:
          countRejected();
          W_LOG("ST: frame from servicetool too long, dropped");
          break;
        default:
          break;
        }
      }
      if (!alive)
      {
        reason = "send to client failed";
        break;
      }
    }
    if (inflight)
      inflight->abort = true;
    endSession(reason);
  }

  void TaskServicetool(void * /*pvParameters*/)
  {
    int listenFd = -1;
    uint16_t listenPort = 0;
    waitPhase(PHASE_CONFIG | PHASE_NET); // needs loaded config and the network

    uint8_t lastHaUnavail = systemConfig.servicetool_ha_unavail;
    uint8_t lastEnabled = systemConfig.servicetool_bridge;

    for (;;)
    {
      // HA discovery contents depend on these settings: resend on change
      if (systemConfig.servicetool_ha_unavail != lastHaUnavail || systemConfig.servicetool_bridge != lastEnabled)
      {
        lastHaUnavail = systemConfig.servicetool_ha_unavail;
        lastEnabled = systemConfig.servicetool_bridge;
        sendHomeAssistantDiscovery = true;
        servicetoolMqttUpdate = true;
      }

      if (systemConfig.servicetool_bridge != 1 || systemConfig.servicetool_port == 0)
      {
        if (listenFd >= 0)
        {
          closeFd(listenFd);
          I_LOG("ST: servicetool bridge stopped");
        }
        servicetoolEndRequested = false;
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        continue;
      }

      if (listenFd < 0 || listenPort != systemConfig.servicetool_port)
      {
        closeFd(listenFd);
        listenPort = systemConfig.servicetool_port;
        listenFd = openListenSocket(listenPort);
        if (listenFd < 0)
        {
          E_LOG("ST: error - could not listen on TCP port %u", listenPort);
          vTaskDelay(5000 / portTICK_PERIOD_MS);
          continue;
        }
        I_LOG("ST: servicetool bridge listening on TCP port %u", listenPort);
      }

      servicetoolEndRequested = false; // only meaningful during a session

      int r = waitReadable(listenFd, 500);
      if (r <= 0)
      {
        if (r < 0)
          closeFd(listenFd); // reopen next round
        continue;
      }

      struct sockaddr_in peer;
      socklen_t plen = sizeof(peer);
      int clientFd = accept(listenFd, reinterpret_cast<struct sockaddr *>(&peer), &plen);
      if (clientFd < 0)
        continue;

      char ip[16]{};
      inet_ntoa_r(peer.sin_addr, ip, sizeof(ip));

      if (systemConfig.servicetool_allow_ip[0] != '\0' && strcmp(systemConfig.servicetool_allow_ip, ip) != 0)
      {
        W_LOG("ST: refused servicetool connection from %s (not the allowed IP)", ip);
        closeFd(clientFd);
        continue;
      }
      if (systemConfig.itho_rf_standalone == 1)
      {
        W_LOG("ST: refused servicetool connection - RF standalone mode has no I2C connection");
        closeFd(clientFd);
        continue;
      }

      configureClientSocket(clientFd);
      startSession(ip);
      serveClient(clientFd);
      closeFd(clientFd);
    }
  }

} // namespace

// ---------------------------------------------------------------------------
// public API
// ---------------------------------------------------------------------------

void startTaskServicetoolBridge()
{
  if (taskHandle != nullptr)
    return;
  if (infoMutex == nullptr)
    infoMutex = xSemaphoreCreateMutex();

  xTaskCreatePinnedToCore(
      TaskServicetool,
      "TaskServicetool",
      STACK_SIZE_MEDIUM,
      nullptr,
      TASK_SERVICETOOL_PRIO,
      &taskHandle,
      CONFIG_ARDUINO_RUNNING_CORE);
}

void servicetoolNoteBlocked(i2c_cmdref_t origin)
{
  uint32_t n = ++blockedCount;
  if (n <= SERVICETOOL_BLOCK_LOG_LIMIT)
    W_LOG("ST: I2C command blocked during servicetool session (origin:%d)", static_cast<int>(origin));
  else if (n == SERVICETOOL_BLOCK_LOG_LIMIT + 1)
    W_LOG("ST: further blocked I2C commands are not logged individually");
}

void servicetoolGetStatusJSON(JsonObject obj, bool full)
{
  bool active = servicetoolSessionActive;
  obj["state"] = active ? "active" : "inactive";
  obj["enabled"] = systemConfig.servicetool_bridge == 1;
  if (!active)
    return;

  lockInfo();
  SessionInfo copy = info;
  unlockInfo();

  obj["client"] = copy.client;
  obj["since"] = copy.since;
  obj["last_activity"] = copy.lastActivity;
  obj["timeout_min"] = systemConfig.servicetool_timeout;
  if (full)
  {
    obj["frames_tx"] = copy.framesTx;
    obj["frames_rx"] = copy.framesRx;
    obj["frames_rejected"] = copy.framesRejected;
    obj["blocked_commands"] = static_cast<uint32_t>(blockedCount);
  }
}

void servicetoolMqttTopic(char *buf, size_t len)
{
  snprintf(buf, len, "%s%s", systemConfig.mqtt_base_topic, "/servicetool");
}
