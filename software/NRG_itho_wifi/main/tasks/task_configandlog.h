#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"

#include <Arduino.h>
#include <Ticker.h>
#include "ArduinoNvs.h"

#include "config/SystemConfig.h"
#include "config/WifiConfig.h"
#include "globals.h"
#include "sys_log.h"
#include "notifyClients.h"

#include "tasks/task_syscontrol.h"
#include "config/Config.h"
#include "config/LogConfig.h"
#include "config/HADiscConfig.h"
#include "esp32_repartition.h"

// #include "LittleFS.h"
#include "uuid.h"

// globals
extern Ticker TaskConfigAndLogTimeout;
extern TaskHandle_t xTaskConfigAndLogHandle;
extern uint32_t TaskConfigAndLogHWmark;
extern volatile bool saveRemotesflag;
extern volatile bool saveVremotesflag;
extern bool chkpartition;
extern int chk_partition_res;
extern bool formatFileSystem;
extern bool flashLogInitReady;
extern char uuid[UUID_STR_LEN];

inline bool RemotesConfigLoaded = false;
inline bool VirtualRemotesConfigLoaded = false;
inline bool WifiConfigLoaded = false;
inline bool SystemConfigLoaded = false;
inline bool logConfigLoaded = false;
inline bool HADiscConfigLoaded = false;
inline bool saveAllConfigsflag = false;

void startTaskConfigAndLog();
void TaskConfigAndLog(void *pvParameters);
void execLogAndConfigTasks();

void syslog_queue_worker();
bool initFileSystem();
void logInit();
void log_mem_info();
