/*
   I2C Manager - Centralized I2C communication state and queue management
   Part of NRG itho wifi firmware
*/

#pragma once

#include <Arduino.h>
#include <deque>
#include <functional>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "../ithodevice/i2c_logger.h"

#define I2C_SLAVE_RX_BUF_LEN 512

class I2CManager
{
private:
  SemaphoreHandle_t queueMutex; // Thread safety for queue access

public:
  // Pin configuration
  gpio_num_t master_sda_pin;
  gpio_num_t master_scl_pin;
  gpio_num_t slave_sda_pin;
  gpio_num_t slave_scl_pin;
  gpio_num_t sniffer_sda_pin;
  gpio_num_t sniffer_scl_pin;

  // Command queue
  std::deque<std::function<void()>> cmd_queue;

  // Communication buffer
  uint8_t buffer[I2C_SLAVE_RX_BUF_LEN];

  // Safe guard state (void* to avoid circular dependency, actual type is i2c_safe_guard_t*)
  void *safe_guard;

  // Logger
  I2CLogger logger;

  I2CManager();
  ~I2CManager();

  /**
   * @brief Add a command function to the I2C queue
   * @param func Function to execute
   */
  void queueAdd(const std::function<void()> &func);

  /**
   * @brief Process the next command in the I2C queue
   * Includes safe guard logic to prevent interference with temperature readout
   */
  void processQueue();

  /**
   * @brief Check if the queue is empty
   * @return true if queue is empty, false otherwise
   */
  bool queueEmpty() const;

  /**
   * @brief Set I2C master pins
   */
  void setMasterPins(gpio_num_t sda, gpio_num_t scl);

  /**
   * @brief Set I2C slave pins
   */
  void setSlavePins(gpio_num_t sda, gpio_num_t scl);

  /**
   * @brief Set I2C sniffer pins
   */
  void setSnifferPins(gpio_num_t sda, gpio_num_t scl);
};

extern I2CManager i2cManager;
