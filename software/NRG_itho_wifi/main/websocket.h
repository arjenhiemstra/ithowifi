#pragma once

#include <Arduino.h>

#include <string.h>

#include <ArduinoJson.h>

#include "globals.h"

#include "SystemConfig.h"
#include "WifiConfig.h"

void websocketInit();
void jsonWsSend(const char *rootName);
