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
#include <HTTPClient.h>

// #include "LittleFS.h"

// globals
extern uint32_t TaskSysControlHWmark;
extern bool runscan;
extern bool saveSystemConfigflag;
extern bool saveLogConfigflag;
extern bool saveHADiscConfigflag;
extern bool saveWifiConfigflag;
extern bool resetWifiConfigflag;
extern bool resetSystemConfigflag;
extern bool resetHADiscConfigflag;
extern bool clearQueue;
extern bool shouldReboot;
extern bool restest;
extern unsigned long getFWupdateInfo;


struct firmwareinfo
{
    int fw_update_available{0};
    char latest_fw[16]{};
    char latest_beta_fw[16]{};
};

extern firmwareinfo firmwareInfo;

void startTaskSysControl();
void TaskSysControl(void *pvParameters);
void execSystemControlTasks();
void updateQueue();
bool writeIthoVal(uint16_t value, volatile uint16_t *ithoCurrentVal, bool *updateIthoMQTT);
void ithoI2CCommand(uint8_t remoteIndex, const char *command, cmdOrigin origin);

#include "tasks/WiFiConnectionManager.h"
#include "tasks/I2CQueryHandlers.h"
#include "tasks/SensorManager.h"
