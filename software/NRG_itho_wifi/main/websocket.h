#pragma once

#include <Arduino.h>

#include <string.h>

#include <ArduinoJson.h>

#include "globals.h"

#include "LittleFS.h"

#include "HADiscovery.h"

#include "config/SystemConfig.h"
#include "config/WifiConfig.h"
#include "config/HADiscConfig.h"

#if defined MG_ENABLE_PACKED_FS && MG_ENABLE_PACKED_FS == 1
#else
#include <MycilaWebSerial.h>
#endif

void websocketInit();
void jsonWsSend(const char *rootName);
