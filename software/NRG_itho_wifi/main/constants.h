#pragma once
#include <stdint.h>

// Task watchdog timeouts (ms)
constexpr uint32_t TASK_SYSCONTROL_TIMEOUT_MS = 35000;
constexpr uint32_t TASK_WEB_TIMEOUT_MS        = 15000;
constexpr uint32_t TASK_MQTT_TIMEOUT_MS       = 35000;
constexpr uint32_t TASK_CONFIGLOG_TIMEOUT_MS  = 15000;

// Reconnect intervals (ms)
constexpr unsigned long WIFI_RECONNECT_INTERVAL_MS = 60000;
constexpr unsigned long MQTT_RECONNECT_INTERVAL_MS =  5000;

// I2C init timing (ms)
constexpr unsigned long I2C_INIT_DELAY_MS           = 15000;
constexpr unsigned long I2C_INIT_RETRY_INTERVAL_MS  =  5000;
// First-generation CVE ECO 2 SP (type:0x04 fw:0x01) drops out of
// PWM-accept mode whenever a single init frame is missed; 2.8.1 kept
// it in mode by re-sending the handshake every loop tick. Re-send on
// a slow timer to keep that unit responsive without flooding the bus.
constexpr unsigned long I2C_PWM_INIT_RESEND_INTERVAL_MS = 30000;

// Itho device IDs
constexpr uint8_t ITHO_DEVICE_CVE        = 0x04;
constexpr uint8_t ITHO_DEVICE_RFT        = 0x14;
constexpr uint8_t ITHO_DEVICE_CVE_SILENT = 0x1B;
constexpr uint8_t ITHO_DEVICE_RFT_CVE    = 0x1D;
