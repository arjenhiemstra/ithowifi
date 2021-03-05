
#include <driver/i2c.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <string>
#include <driver/gpio.h>
#include "hardware.h"

#define I2C_SLAVE_SDA_IO     SDAPIN
#define I2C_SLAVE_SCL_IO     SCLPIN
#define I2C_SLAVE_SDA_PULLUP GPIO_PULLUP_DISABLE
#define I2C_SLAVE_SCL_PULLUP GPIO_PULLUP_DISABLE
#define I2C_SLAVE_NUM        I2C_NUM_1
#define I2C_SLAVE_RX_BUF_LEN 512
#define I2C_SLAVE_ADDRESS    0x40



inline uint8_t checksum(const uint8_t* buf, size_t buflen) {
    uint8_t sum = 0;
    while (buflen--) {
        sum += *buf++;
    }
    return -sum;
}

inline char toHex(uint8_t c) { return c < 10 ? c + '0' : c + 'A' - 10; }
inline bool checksumOk(const uint8_t* data, size_t len) { return checksum(data, len - 1) == data[len - 1]; }

portMUX_TYPE status_mux = portMUX_INITIALIZER_UNLOCKED;
std::string datatypes;

static const char* TAG = "i2c-slave";

static i2c_slave_callback_t i2c_callback;
static uint8_t buf[I2C_SLAVE_RX_BUF_LEN];
static size_t buflen;



static void i2c_slave_task(void* arg) {
    while (1) {
        buf[0] = I2C_SLAVE_ADDRESS << 1;
        buflen = 1;
        while (1) {
            int len1 = i2c_slave_read_buffer(I2C_SLAVE_NUM, buf + buflen, sizeof(buf) - buflen, 10);
            if (len1 <= 0)
                break;
            buflen += len1;
        }
        if (buflen > 1) {
            i2c_callback(buf, buflen);
        }
    }
}

void i2c_slave_init(i2c_slave_callback_t cb) {
    i2c_config_t conf = {I2C_MODE_SLAVE,   (gpio_num_t)I2C_SLAVE_SDA_IO,     I2C_SLAVE_SDA_PULLUP,
                         (gpio_num_t)I2C_SLAVE_SCL_IO, I2C_SLAVE_SCL_PULLUP, {.slave = {0, I2C_SLAVE_ADDRESS}}};
    i2c_param_config(I2C_SLAVE_NUM, &conf);
    i2c_callback = cb;
    i2c_driver_install(I2C_SLAVE_NUM, conf.mode, I2C_SLAVE_RX_BUF_LEN, 0, 0);
    ESP_LOGI(TAG, "Slave pin assignment: SCL=%d, SDA=%d", I2C_SLAVE_SCL_IO, I2C_SLAVE_SDA_IO);
    ESP_LOGI(TAG, "Slave address: %02X (W)", I2C_SLAVE_ADDRESS << 1);
    xTaskCreatePinnedToCore(i2c_slave_task, "i2c_task", 4096, NULL, 22, NULL, 0);
}


void i2c_slave_callback(const uint8_t* data, size_t len) {
    std::string s;
    s.reserve(len * 3 + 2);
    for (size_t i = 0; i < len; ++i) {
        if (i)
            s += ' ';
        s += toHex(data[i] >> 4);
        s += toHex(data[i] & 0xF);
    }
    if (len > 6 && data[1] == 0x82 && data[2] == 0xA4 && data[3] == 0 && data[4] == 1 && data[5] < len - 5 && checksumOk(data, len)) {
        portENTER_CRITICAL(&status_mux);
        datatypes.assign((const char*)data + 6, data[5]);
        portEXIT_CRITICAL(&status_mux);
    }
    if (len > 6 && data[1] == 0x82 && data[2] == 0xa4 && data[3] == 1 && data[4] == 1 && data[5] < len - 5 && checksumOk(data, len)) {
        //handleStatus(data + 6, len - 7);
    }
    
    sprintf(i2cresult, "Slave: %s\n", s.c_str());
    callback_called = true;
    
}
