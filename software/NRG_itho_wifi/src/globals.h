#pragma once

#include "version.h"

#ifdef ESPRESSIF32_3_5_0
#define ACTIVE_FS LITTLEFS
#else
#define ACTIVE_FS LittleFS
#endif

#define STACK_SIZE_SMALL 2048
#define STACK_SIZE 4096
#define STACK_SIZE_MEDIUM 6144
#define STACK_SIZE_LARGE 8192

#if defined(ESP32)
#else
#error "Usupported hardware"
#endif

#define LOGGING_INTERVAL 21600000 // Log system status at regular intervals
#define ENABLE_FAILSAVE_BOOT
//#define ENABLE_SERIAL

#include "task_mqtt.h"
#include "task_web.h"
#include "task_init.h"
#include "task_cc1101.h"

#include "esp_wifi.h"
#include "WiFiClient.h"

#include "IthoCC1101.h" // Largly based on and thanks to https://github.com/supersjimmie/IthoEcoFanRFT
#include "IthoPacket.h" // Largly based on and thanks to https://github.com/supersjimmie/IthoEcoFanRFT
#include "IthoRemote.h"

extern const char *WiFiAPPSK;

extern bool i2c_sniffer_capable;
extern uint8_t hardware_rev_det;

extern gpio_num_t wifi_led_pin;
extern gpio_num_t status_pin;
extern gpio_num_t boot_state_pin;
extern gpio_num_t itho_irq_pin;
extern gpio_num_t fail_save_pin;
extern gpio_num_t itho_status_pin;

extern const char *cve2;
extern const char *non_cve1;
extern const char *hw_revision;

extern WiFiClient client;

extern IthoCC1101 rf;
extern IthoPacket packet;

