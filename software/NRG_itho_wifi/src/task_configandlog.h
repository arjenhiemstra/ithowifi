#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"

#include <Arduino.h>
#include <Ticker.h>

#include "SystemConfig.h"
#include "WifiConfig.h"
#include "hardware.h"
#include "globals.h"
#include "Dbglog.h"
#include "flashLog.h"
#include "notifyClients.h"

#include "task_syscontrol.h"
#include "Config.h"

// globals
extern Ticker TaskConfigAndLogTimeout;
extern TaskHandle_t xTaskConfigAndLogHandle;
extern uint32_t TaskConfigAndLogHWmark;
extern volatile bool saveRemotesflag;
extern volatile bool saveVremotesflag;
extern bool formatFileSystem;

void startTaskConfigAndLog();
void TaskConfigAndLog(void *pvParameters);
void execLogAndConfigTasks();

bool initFileSystem();
void logInit();
