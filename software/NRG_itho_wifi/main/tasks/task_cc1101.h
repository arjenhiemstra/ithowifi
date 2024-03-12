#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"

#include <Arduino.h>
#include <Ticker.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>

#include "config/SystemConfig.h"
#include "config/WifiConfig.h"
#include "globals.h"
#include "generic_functions.h"
#include "sys_log.h"
#include "notifyClients.h"
#include "IthoSystem.h"

#include "cc1101/IthoPacket.h"
#include "tasks/task_configandlog.h"

#include "LittleFS.h"

// globals
extern Ticker TaskCC1101Timeout;
extern TaskHandle_t xTaskCC1101Handle;
extern uint32_t TaskCC1101HWmark;
extern uint8_t debugLevel;

IRAM_ATTR void ITHOinterrupt();
void disableRFsupport();
uint8_t findRFTlastCommand();
void RFDebug(IthoCommand cmd);
void toggleRemoteLLmode(const char *remotetype);
void setllModeTimer();
void startTaskCC1101();
void TaskCC1101(void *pvParameters);
