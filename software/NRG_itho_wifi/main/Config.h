// Config.h

#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"

#include "IthoRemote.h"
#include "globals.h"
#include "sys_log.h"



bool loadWifiConfig();
bool saveWifiConfig();
bool resetWifiConfig();
bool loadSystemConfig();
bool saveSystemConfig();
bool resetSystemConfig();
uint16_t serializeRemotes(const char *filename, const IthoRemote &remotes, Print &dst);
bool saveRemotesConfig();
bool saveVirtualRemotesConfig();
bool saveFileRemotes(const char *filename, const IthoRemote &remotes);
bool deserializeRemotes(const char *filename, Stream &src, IthoRemote &remotes);
bool loadRemotesConfig();
bool loadVirtualRemotesConfig();
bool loadFileRemotes(const char *filename, IthoRemote &remotes);
bool resetLogConfig();
bool saveLogConfig();
bool loadLogConfig();
