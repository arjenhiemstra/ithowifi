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

// #include "LittleFS.h"

// globals
extern Ticker TaskCC1101Timeout;
extern TaskHandle_t xTaskCC1101Handle;
extern uint32_t TaskCC1101HWmark;
extern bool send31D9;
extern bool send31D9debug;
extern bool send31DAdebug;
extern uint8_t status31D9;
extern bool fault31D9;
extern bool frost31D9;
extern bool filter31D9;

extern bool send31DA;
extern uint8_t faninfo31DA;
extern uint8_t timer31DA;

void ITHOinterrupt();
void disableRF_ISR();
void enableRF_ISR();
void RFDebug(IthoCommand cmd);
void toggleRemoteLLmode(const char *remotetype);
void setllModeTimer();
void startTaskCC1101();
void TaskCC1101(void *pvParameters);
