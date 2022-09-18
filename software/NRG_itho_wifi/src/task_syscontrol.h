#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"

#include <Arduino.h>
#include <DNSServer.h>

#include "SystemConfig.h"
#include "WifiConfig.h"
#include "hardware.h"
#include "globals.h"
#include "Dbglog.h"
#include "flashLog.h"
#include "notifyClients.h"
#include "SHTSensor.h"
#include "task_mqtt.h"
#include "enum.h"

// globals
extern uint32_t TaskSysControlHWmark;
extern DNSServer dnsServer;
extern bool SHT3x_original;
extern bool SHT3x_alternative;
extern bool runscan;
extern bool dontSaveConfig;
extern bool saveSystemConfigflag;
extern bool saveWifiConfigflag;
extern bool resetWifiConfigflag;
extern bool resetSystemConfigflag;
extern bool clearQueue;
extern bool shouldReboot;
extern int8_t ithoInitResult;
extern bool IthoInit;

void startTaskSysControl();
void TaskSysControl(void *pvParameters);
void execSystemControlTasks();
void wifiInit();
void setupWiFiAP();
bool connectWiFiSTA();
void initSensor();
void init_vRemote();
bool ithoInitCheck();
void update_queue();
bool writeIthoVal(uint16_t value, volatile uint16_t *ithoCurrentVal, bool *updateIthoMQTT);
bool ithoI2CCommand(uint8_t remoteIndex, const char *command, cmdOrigin origin);
