#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"

#include <Arduino.h>
#include <Ticker.h>
#include <ArduinoOTA.h>

#include "SystemConfig.h"
#include "WifiConfig.h"
#include "hardware.h"
#include "globals.h"
#include "Dbglog.h"
#include "flashLog.h"
#include "notifyClients.h"

// globals
extern bool TaskInitReady;

void TaskInit(void *pvParameters);

#if defined(ENABLE_FAILSAVE_BOOT)
void failSafeBoot();
#endif

void hardwareInit();
void i2cInit();
