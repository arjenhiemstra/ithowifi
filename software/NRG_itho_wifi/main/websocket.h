#pragma once

#include <Arduino.h>

#include <string.h>

#include <ArduinoJson.h>

#include "globals.h"

#ifdef ESPRESSIF32_3_5_0
#include <LITTLEFS.h>
#else
#include "LittleFS.h"
#endif

#include "SystemConfig.h"
#include "WifiConfig.h"

void websocketInit();
void jsonWsSend(const char *rootName);
