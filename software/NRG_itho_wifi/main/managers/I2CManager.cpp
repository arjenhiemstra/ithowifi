/*
   I2C Manager - Implementation
   Part of NRG itho wifi firmware
*/

#include "I2CManager.h"
#include "../ithodevice/i2c_sniffer.h"

// Global instance
I2CManager i2cManager;

I2CManager::I2CManager()
    : master_sda_pin(GPIO_NUM_0),
      master_scl_pin(GPIO_NUM_0),
      slave_sda_pin(GPIO_NUM_0),
      slave_scl_pin(GPIO_NUM_0),
      sniffer_sda_pin(GPIO_NUM_1),
      sniffer_scl_pin(GPIO_NUM_1),
      safe_guard(nullptr)
{
  // Allocate and initialize safe guard
  auto *sg = new i2c_safe_guard_t();
  sg->start_close_time = 0;
  sg->end_close_time = 0;
  sg->sensor_update_freq_setting = 8000;
  sg->sniffer_enabled = false;
  sg->sniffer_web_enabled = false;
  sg->i2c_safe_guard_enabled = false;
  safe_guard = sg;

  // Create mutex for thread-safe queue access
  queueMutex = xSemaphoreCreateMutex();

  // Initialize buffer
  memset(buffer, 0, I2C_SLAVE_RX_BUF_LEN);
}

I2CManager::~I2CManager()
{
  if (queueMutex != NULL)
  {
    vSemaphoreDelete(queueMutex);
  }

  if (safe_guard != nullptr)
  {
    delete static_cast<i2c_safe_guard_t *>(safe_guard);
  }
}

void I2CManager::queueAdd(const std::function<void()> &func)
{
  if (queueMutex != NULL && xSemaphoreTake(queueMutex, portMAX_DELAY) == pdTRUE)
  {
    cmd_queue.push_back(func);
    xSemaphoreGive(queueMutex);
  }
  else
  {
    // Fallback if mutex not available (shouldn't happen)
    cmd_queue.push_back(func);
  }
}

void I2CManager::processQueue()
{
  // Check if queue is empty (thread-safe check)
  bool hasCommands = false;
  if (queueMutex != NULL && xSemaphoreTake(queueMutex, 0) == pdTRUE)
  {
    hasCommands = !cmd_queue.empty();
    xSemaphoreGive(queueMutex);
  }
  else
  {
    hasCommands = !cmd_queue.empty();
  }

  if (!hasCommands)
  {
    return;
  }

  // Check safe guard - block 500ms before and after expected temp readout
  if (safe_guard != nullptr)
  {
    auto *sg = static_cast<i2c_safe_guard_t *>(safe_guard);
    if (sg->i2c_safe_guard_enabled && !sg->sniffer_enabled)
    {
      unsigned long cur_time = millis();
      if (sg->start_close_time < cur_time && cur_time < sg->end_close_time)
      {
        // Safe guard active - don't process queue now
        return;
      }
    }
  }

  // Get and execute the next command
  std::function<void()> func;
  bool gotCommand = false;

  if (queueMutex != NULL && xSemaphoreTake(queueMutex, portMAX_DELAY) == pdTRUE)
  {
    if (!cmd_queue.empty())
    {
      func = cmd_queue.front();
      cmd_queue.pop_front();
      gotCommand = true;
    }
    xSemaphoreGive(queueMutex);
  }

  if (gotCommand && func)
  {
    func(); // Execute the command outside of the mutex
  }
}

bool I2CManager::queueEmpty() const
{
  if (queueMutex != NULL && xSemaphoreTake(queueMutex, 0) == pdTRUE)
  {
    bool empty = cmd_queue.empty();
    xSemaphoreGive(queueMutex);
    return empty;
  }
  return cmd_queue.empty();
}

void I2CManager::setMasterPins(gpio_num_t sda, gpio_num_t scl)
{
  master_sda_pin = sda;
  master_scl_pin = scl;
}

void I2CManager::setSlavePins(gpio_num_t sda, gpio_num_t scl)
{
  slave_sda_pin = sda;
  slave_scl_pin = scl;
}

void I2CManager::setSnifferPins(gpio_num_t sda, gpio_num_t scl)
{
  sniffer_sda_pin = sda;
  sniffer_scl_pin = scl;
}
