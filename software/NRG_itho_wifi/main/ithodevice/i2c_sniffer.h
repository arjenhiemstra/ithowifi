#pragma once
#include <driver/gpio.h>
#include "hal/gpio_ll.h"
#include "soc/gpio_struct.h"
#include <esp32/rom/ets_sys.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <string>
#include <ArduinoJson.h>
#include <Ticker.h>

#include "IthoDevice.h"
#include "notifyClients.h"
#include "i2c_logger.h"
#include "sys_log.h"

#define I2C_SNIFFER_SDA_PIN GPIO_NUM_1
#define I2C_SNIFFER_SCL_PIN GPIO_NUM_1
#define I2C_SNIFFER_RUN_ON_CORE CONFIG_ARDUINO_RUNNING_CORE
#define I2C_SNIFFER_PRINT_TIMING 0

typedef struct
{
    unsigned long start_close_time;
    unsigned long end_close_time;
    unsigned long sensor_update_freq_setting{8000};
    bool sniffer_enabled;
    bool sniffer_web_enabled;
    bool i2c_safe_guard_enabled;
    void update_time()
    {
        start_close_time = millis() + (sensor_update_freq_setting - 500);
        end_close_time = millis() + (sensor_update_freq_setting + 500);
    }
    void set_update_freq_setting(unsigned long value) { sensor_update_freq_setting = value; }
} i2c_safe_guard_t;

void i2cSnifferProcessBuf(std::string &buffer);
void i2cSafeGuardControl();
void i2cSnifferSetPins(gpio_num_t sda, gpio_num_t scl);
void i2cSnifferInit(bool enabled);
void i2cSnifferEnable();
void i2cSnifferDisable();
void i2cSnifferUnload();
void i2cSnifferPullup(bool enable);