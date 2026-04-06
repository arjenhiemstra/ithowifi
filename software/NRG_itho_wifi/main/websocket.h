#pragma once

#include <Arduino.h>

#include <string.h>
#include <unordered_map>

#include <ArduinoJson.h>

#include "globals.h"

// #include "LittleFS.h"

#include "HADiscovery.h"

#include "config/SystemConfig.h"
#include "config/WifiConfig.h"
#include "config/HADiscConfig.h"

#include <MycilaWebSerial.h>
extern WebSerial* webSerial;

void websocketInit();
void jsonWsSend(const char *rootName);
