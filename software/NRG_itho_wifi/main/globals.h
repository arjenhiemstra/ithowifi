#pragma once

#include "version.h"
#include "sdkconfig.h"
#include "constants.h"

#define ACTIVE_FS LittleFS

#define STACK_SIZE_SMALL 2048
#define STACK_SIZE 4096
#define STACK_SIZE_MEDIUM 6144
#define STACK_SIZE_LARGE 8192
#define STACK_SIZE_XLARGE 16384

#if defined(ESP32)
#else
#error "Usupported hardware"
#endif

#define LOGGING_INTERVAL 21600000 // Log system status at regular intervals
// #define ENABLE_SERIAL

#include "tasks/task_mqtt.h"
#include "tasks/task_web.h"
#include "tasks/task_init.h"
#include "tasks/task_cc1101.h"

#include "esp_wifi.h"
#include "WiFiClient.h"
#include "WiFiClientSecure.h"

#include "cc1101/IthoCC1101.h" // Largly based on and thanks to https://github.com/supersjimmie/IthoEcoFanRFT
#include "cc1101/IthoPacket.h" // Largly based on and thanks to https://github.com/supersjimmie/IthoEcoFanRFT
#include "config/IthoRemote.h"

// Manager classes (Phase 1, 2 & 3 refactoring - all shims removed)
#include "managers/HardwareManager.h"
#include "managers/I2CManager.h"
#include "managers/NetworkManager.h"
#include "managers/RFManager.h"
