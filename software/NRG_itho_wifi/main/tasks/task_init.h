#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"

#include <Arduino.h>
#include <Ticker.h>
#include <ArduinoOTA.h>

#include "config/SystemConfig.h"
#include "config/WifiConfig.h"
#include "globals.h"
#include "sys_log.h"
#include "notifyClients.h"
#include "tasks/task_configandlog.h"
#include "i2c_sniffer.h"

// globals
extern bool TaskInitReady;

void TaskInit(void *pvParameters);

void failSafeBoot();
void hardwareInit();
