#pragma once
#include <driver/gpio.h>
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

#include "IthoSystem.h"
#include "notifyClients.h"
#include "i2c_logger.h"
#include "sys_log.h"

#define I2C_SNIFFER_SDA_PIN GPIO_NUM_1
#define I2C_SNIFFER_SCL_PIN GPIO_NUM_1
#define I2C_SNIFFER_RUN_ON_CORE CONFIG_ARDUINO_RUNNING_CORE
#define I2C_SNIFFER_PRINT_TIMING 0

extern gpio_num_t sniffer_sda_pin;
extern gpio_num_t sniffer_scl_pin;

typedef struct
{
    unsigned long start_close_time;
    unsigned long end_close_time;
    unsigned long sensor_update_freq_setting{8000};
    bool sniffer_enabled;
    bool i2c_safe_guard_enabled;
    void update_time()
    {
        start_close_time = millis() + (sensor_update_freq_setting - 500);
        end_close_time = millis() + (sensor_update_freq_setting + 500);
    }
    void set_update_freq_setting(unsigned long value) { sensor_update_freq_setting = value; }
} i2c_safe_guard_t;

extern i2c_safe_guard_t i2c_safe_guard;

void i2c_sniffer_process_buf(std::string &buffer);
void i2c_safe_guard_control();
void i2c_sniffer_setpins(gpio_num_t sda, gpio_num_t scl);
void i2c_sniffer_init(bool enabled);
void i2c_sniffer_enable();
void i2c_sniffer_disable();
void i2c_sniffer_unload();
void i2c_sniffer_pullup(bool enable);