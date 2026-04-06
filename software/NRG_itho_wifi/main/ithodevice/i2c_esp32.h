#pragma once

#include <string>

#include <Arduino.h>
#include <cstdio>
#include <driver/i2c.h>
#include <esp_log.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <queue>
//#include <functional>

#include "IthoDevice.h"
#include "i2c_logger.h"
#include "managers/I2CManager.h"

#define I2C_CMD_QUEUE_MAX_SIZE 10

#define WRITE_BIT I2C_MASTER_WRITE  /*!< I2C master write */
#define READ_BIT I2C_MASTER_READ    /*!< I2C master read */
#define ACK_CHECK_EN 0x1            /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0           /*!< I2C master will not check ack from slave */
#define ACK_VAL 0x0                 /*!< I2C ack value */
#define NACK_VAL 0x1                /*!< I2C nack value */
#define I2C_MASTER_TX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */

#define I2C_MASTER_SDA_PULLUP GPIO_PULLUP_DISABLE
#define I2C_MASTER_SCL_PULLUP GPIO_PULLUP_DISABLE
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 100000

#define I2C_SLAVE_SDA_PULLUP GPIO_PULLUP_DISABLE
#define I2C_SLAVE_SCL_PULLUP GPIO_PULLUP_DISABLE
#define I2C_SLAVE_NUM I2C_NUM_0
#define I2C_SLAVE_EXPECTED_MAX_FREQ_HZ 400000
#define I2C_SLAVE_RX_BUF_LEN 512
#define I2C_SLAVE_TX_BUF_LEN 512
#define I2C_SLAVE_ADDRESS 0x40

#include <driver/gpio.h>
#include <stdint.h>

struct cmd_queue_data
{
    i2c_cmdref_t cmd{};
    uint8_t index{};
    int32_t value{};
    bool update_state{};
    
    bool loop{};
    uint16_t *ithoCurrentVal{};
    bool *updateIthoMQTT{};
};

void i2cQueueAddCmd(const std::function<void()> func);

void i2cMasterSetPins(gpio_num_t sda, gpio_num_t scl);
void i2cSlaveSetPins(gpio_num_t sda, gpio_num_t scl);
esp_err_t i2cMasterInit();
void i2cMasterDeinit();
esp_err_t i2cMasterSend(const char *buf, uint32_t len, int log_entry_idx);
esp_err_t i2cMasterSendCommand(uint8_t addr, const uint8_t *cmd, uint32_t len, int log_entry_idx);

esp_err_t i2cMasterReadSlave(uint8_t addr, uint8_t *data_rd, size_t size, i2c_cmdref_t origin);
esp_err_t i2cMasterReadSlave(uint8_t addr, uint8_t *data_rd, size_t size);
bool i2cSendBytes(const uint8_t *buf, size_t len, i2c_cmdref_t origin);
bool i2cSendBytes(const uint8_t *buf, size_t len);
bool i2cSendCmd(uint8_t addr, const uint8_t *cmd, size_t len, i2c_cmdref_t origin);
bool i2cSendCmd(uint8_t addr, const uint8_t *cmd, size_t len);

size_t i2cSlaveReceive(uint8_t *i2c_receive_buf);
void i2cSlaveDeinit();
bool checkI2Cbus(int log_entry_idx);
int i2cClearBus();
void triggerShtSensorReset();
