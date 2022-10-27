#pragma once

#include <string>

#include <Arduino.h>
#include <stdio.h>
#include <driver/i2c.h>
#include <esp_log.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include "hardware.h"
#include "Dbglog.h"
#include "IthoSystem.h"

#define WRITE_BIT I2C_MASTER_WRITE  /*!< I2C master write */
#define READ_BIT I2C_MASTER_READ    /*!< I2C master read */
#define ACK_CHECK_EN 0x1            /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0           /*!< I2C master will not check ack from slave */
#define ACK_VAL 0x0                 /*!< I2C ack value */
#define NACK_VAL 0x1                /*!< I2C nack value */
#define I2C_MASTER_TX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */

#if defined(CVE)
#define I2C_MASTER_SDA_IO GPIO_NUM_21
#define I2C_MASTER_SCL_IO GPIO_NUM_22
#elif defined(NON_CVE)
#define I2C_MASTER_SDA_IO GPIO_NUM_27
#define I2C_MASTER_SCL_IO GPIO_NUM_26
#endif
#define I2C_MASTER_SDA_PULLUP GPIO_PULLUP_DISABLE
#define I2C_MASTER_SCL_PULLUP GPIO_PULLUP_DISABLE
#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 100000

#if defined(CVE)
#define I2C_SLAVE_SDA_IO GPIO_NUM_21
#define I2C_SLAVE_SCL_IO GPIO_NUM_22
#elif defined(NON_CVE)
#define I2C_SLAVE_SDA_IO GPIO_NUM_27
#define I2C_SLAVE_SCL_IO GPIO_NUM_26
#endif

#define I2C_SLAVE_SDA_PULLUP GPIO_PULLUP_DISABLE
#define I2C_SLAVE_SCL_PULLUP GPIO_PULLUP_DISABLE
#define I2C_SLAVE_NUM I2C_NUM_0
#define I2C_SLAVE_RX_BUF_LEN 512
#define I2C_SLAVE_TX_BUF_LEN 512
#define I2C_SLAVE_ADDRESS 0x40

#include <driver/gpio.h>
#include <stdint.h>

extern uint8_t i2cbuf[I2C_SLAVE_RX_BUF_LEN];

char toHex(uint8_t c);

esp_err_t i2c_master_init();
void i2c_master_deinit();
esp_err_t i2c_master_send(const char *buf, uint32_t len);
esp_err_t i2c_master_send_command(uint8_t addr, const uint8_t *cmd, uint32_t len);
esp_err_t i2c_master_read_slave(uint8_t addr, uint8_t *data_rd, size_t size);
bool i2c_sendBytes(const uint8_t *buf, size_t len);
bool i2c_sendCmd(uint8_t addr, const uint8_t *cmd, size_t len);

size_t i2c_slave_receive(uint8_t i2c_receive_buf[]);
void i2c_slave_deinit();
bool checkI2Cbus();
int I2C_ClearBus();