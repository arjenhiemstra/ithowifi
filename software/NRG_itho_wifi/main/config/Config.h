// Config.h

#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"

#include "config/IthoRemote.h"
#include "globals.h"
#include "sys_log.h"

#include "LittleFS.h"

bool loadWifiConfig(const char *location);
bool saveWifiConfig(const char *location);
bool resetWifiConfig();
bool loadSystemConfig(const char *location);
bool saveSystemConfig(const char *location);
bool resetSystemConfig();
bool loadLogConfig(const char *location);
bool saveLogConfig(const char *location);
bool resetLogConfig();
bool loadHADiscConfig(const char *location);
bool saveHADiscConfig(const char *location);
bool resetHADiscConfig();

template <typename TDst>
uint16_t serializeRemotes(const char *filename, const IthoRemote &remotes, TDst &dst);
bool saveRemotesConfig(const char *location);
bool saveVirtualRemotesConfig(const char *location);
bool saveFileRemotes(const char *location, const char *filename, const char *config_type, const IthoRemote &remotes);
template <typename TDst>
bool deserializeRemotes(const char *filename, const char *config_type, TDst &src, IthoRemote &remotes);
bool loadRemotesConfig(const char *location);
bool loadVirtualRemotesConfig(const char *location);
bool loadFileRemotes(const char *location, const char *filename, const char *config_type, const char *nvs_backup_key, IthoRemote &remotes);
