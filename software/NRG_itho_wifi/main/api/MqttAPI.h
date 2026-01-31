#pragma once

#include <Arduino.h>

void mqttCallback(const char *topic, const byte *payload, unsigned int length);
bool apiCmdAllowed(const char *cmd);
