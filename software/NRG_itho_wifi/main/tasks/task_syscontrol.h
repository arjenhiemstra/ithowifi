#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"

#include <Arduino.h>
#include <DNSServer.h>

#include "config/SystemConfig.h"
#include "config/WifiConfig.h"
#include "globals.h"
#include "sys_log.h"
#include "notifyClients.h"
#include "SHTSensor.h"
#include "tasks/task_mqtt.h"
#include "enum.h"

#include "LittleFS.h"

// globals
extern uint32_t TaskSysControlHWmark;
extern DNSServer dnsServer;
extern bool SHT3x_original;
extern bool SHT3x_alternative;
extern bool runscan;
extern bool dontSaveConfig;
extern bool saveSystemConfigflag;
extern bool saveLogConfigflag;
extern bool saveWifiConfigflag;
extern bool resetWifiConfigflag;
extern bool resetSystemConfigflag;
extern bool clearQueue;
extern bool shouldReboot;
extern int8_t ithoInitResult;
extern bool IthoInit;
extern bool wifiModeAP;
extern bool reset_sht_sensor;
extern bool restest;

void startTaskSysControl();
void TaskSysControl(void *pvParameters);
void update_sht_sensor();
void work_i2c_queue();
void execSystemControlTasks();
void init_i2c_functions();
void wifiInit();
void setupWiFiAP();
bool connectWiFiSTA(bool restore = false);
void set_static_ip_config();
void initSensor();
void initSensor();
void init_vRemote();
bool ithoInitCheck();
void update_queue();
bool writeIthoVal(uint16_t value, volatile uint16_t *ithoCurrentVal, bool *updateIthoMQTT);
void ithoI2CCommand(uint8_t remoteIndex, const char *command, cmdOrigin origin);
