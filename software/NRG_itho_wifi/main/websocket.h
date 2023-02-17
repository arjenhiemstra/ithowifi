#pragma once

#include <Arduino.h>

#include <string.h>

#include <ArduinoJson.h>

#include "globals.h"

#include "LittleFS.h"

#include "config/SystemConfig.h"
#include "config/WifiConfig.h"

void websocketInit();
void jsonWsSend(const char *rootName);
