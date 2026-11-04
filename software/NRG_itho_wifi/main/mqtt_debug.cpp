#include "mqtt_debug.h"

#include <Arduino.h>
#include <atomic>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"

#include "config/SystemConfig.h"
#include "sys_log.h"
#include "tasks/task_mqtt.h"

#define MQTT_DEBUG_BUF_SIZE 4096
#define MQTT_DEBUG_MAX_LINE 512
#define MQTT_DEBUG_IDLE_MS 60000UL
#define MQTT_DEBUG_PER_LOOP 8

// The logs run in other tasks and PubSubClient is not thread safe, so lines go through a ring buffer.
// TaskMQTT creates it when lines arrive and frees it after a minute without lines.
static RingbufHandle_t ringbuf = nullptr;
static std::atomic<bool> active{false};
static std::atomic<int> pushing{0};
static std::atomic<bool> requested{false};
static std::atomic<uint32_t> dropped{0};

void mqttDebugPush(MqttDebugChannel channel, const char *line)
{
  requested = true;
  pushing++; // keeps TaskMQTT from freeing the buffer while in use
  if (active)
  {
    size_t len = strnlen(line, MQTT_DEBUG_MAX_LINE);
    uint8_t *item = nullptr;
    if (xRingbufferSendAcquire(ringbuf, reinterpret_cast<void **>(&item), len + 1, 0) == pdTRUE)
    {
      item[0] = static_cast<uint8_t>(channel);
      memcpy(item + 1, line, len);
      xRingbufferSendComplete(ringbuf, item);
    }
    else
    {
      dropped++;
    }
  }
  pushing--;
}

void mqttDebugService(bool connected)
{
  static bool inUse = false;
  static bool createFailed = false;
  static unsigned long lastRequest = 0;
  static unsigned long lastDropLog = 0;

  if (requested.exchange(false))
  {
    inUse = true;
    lastRequest = millis();
  }
  else if (inUse && millis() - lastRequest >= MQTT_DEBUG_IDLE_MS)
  {
    inUse = false;
  }

  const bool wanted = connected && inUse;
  if (wanted && ringbuf == nullptr && !createFailed)
  {
    ringbuf = xRingbufferCreate(MQTT_DEBUG_BUF_SIZE, RINGBUF_TYPE_NOSPLIT);
    if (ringbuf == nullptr)
    {
      createFailed = true;
      W_LOG("MQTT: no memory for the debug log buffer");
    }
  }
  if (!inUse)
    createFailed = false;

  active = wanted && ringbuf != nullptr;
  if (ringbuf == nullptr)
    return;

  if (!active)
  {
    if (pushing == 0)
    {
      vRingbufferDelete(ringbuf);
      ringbuf = nullptr;
    }
    return;
  }

  char topic[sizeof(systemConfig.mqtt_base_topic) + 12];
  for (int i = 0; i < MQTT_DEBUG_PER_LOOP; i++)
  {
    size_t size = 0;
    auto *item = static_cast<uint8_t *>(xRingbufferReceive(ringbuf, &size, 0));
    if (item == nullptr)
      break;
    snprintf(topic, sizeof(topic), "%s/debug/%s", systemConfig.mqtt_base_topic,
             item[0] == static_cast<uint8_t>(MqttDebugChannel::RF) ? "rf" : "i2c");
    mqttClient.publish(topic, item + 1, size - 1, false);
    vRingbufferReturnItem(ringbuf, item);
  }

  if (dropped > 0 && millis() - lastDropLog >= 10000)
  {
    lastDropLog = millis();
    W_LOG("MQTT: debug log, %u lines dropped", static_cast<unsigned>(dropped.exchange(0)));
  }
}
