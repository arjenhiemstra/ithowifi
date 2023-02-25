
#pragma once

#include <stdio.h>
#include <string>

#include "sys_log.h"
#include "config/Config.h"

#include "freertos/FreeRTOS.h"
#include "esp_ota_ops.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <esp_log.h>
#include "esp_littlefs.h"
#include "ArduinoNvs.h"

void backup_all_configs();
void repartition_device(const char *mode);
void check_partition_tables();
int current_partition_scheme();