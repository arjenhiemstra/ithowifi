#pragma once

#include <stdint.h>

// RF log and I2C sniffer lines, published to <base>/debug/rf and <base>/debug/i2c while that log is on
enum class MqttDebugChannel : uint8_t
{
  RF,
  I2C
};

// Any task. The line is dropped when MQTT is down or the buffer is full.
void mqttDebugPush(MqttDebugChannel channel, const char *line);
// TaskMQTT only
void mqttDebugService(bool connected);
