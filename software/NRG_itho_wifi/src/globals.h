#pragma once

#define LOGGING_INTERVAL 21600000 // Log system status at regular intervals
#define ENABLE_FAILSAVE_BOOT
//#define ENABLE_SERIAL

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_task_wdt.h"

#include <LITTLEFS.h>

#include "task_mqtt.h"
#include "task_web.h"
#include "task_syscontrol.h"
#include "task_init.h"
#include "task_cc1101.h"
#include "task_configandlog.h"

#include "generic_functions.h"

#include "hardware.h"
#include "Dbglog.h"
#include "flashLog.h"

#include "IthoSystem.h"
#include "notifyClients.h"
#include "IthoCC1101.h"

#include <Ticker.h>
#include <string>

#include "IthoQueue.h"
#include "System.h"
#include "SystemConfig.h"
#include "WifiConfig.h"

#include <ArduinoOTA.h>
#include <FS.h>

#include "IthoCC1101.h" // Largly based on and thanks to https://github.com/supersjimmie/IthoEcoFanRFT
#include "IthoPacket.h" // Largly based on and thanks to https://github.com/supersjimmie/IthoEcoFanRFT
#include "IthoRemote.h"

extern const char *WiFiAPPSK;

extern WiFiClient client;

extern IthoCC1101 rf;
extern IthoPacket packet;
extern IthoRemote remotes;
extern IthoRemote virtualRemotes;
