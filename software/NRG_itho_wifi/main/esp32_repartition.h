
#pragma once

#include <cstdio>
#include <string>

#include "sys_log.h"
#include "config/Config.h"

#include "freertos/FreeRTOS.h"
#include "esp_ota_ops.h"
#include "esp_err.h"
#include "esp_flash.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <esp_log.h>
#include "esp_littlefs.h"
#include "ArduinoNvs.h"

void backupAllConfigs();
void repartitionDevice(const char *mode);
void checkPartitionTables();
int currentPartitionScheme();