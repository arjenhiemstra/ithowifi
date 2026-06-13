/*
   Hardware Manager - Centralized hardware detection and GPIO configuration
   Part of NRG itho wifi firmware
*/

#pragma once

#include <Arduino.h>
#include <Ticker.h>

class HardwareManager
{
public:
  // Detection state
  bool i2c_sniffer_capable;
  uint8_t hardware_rev_det;

  // GPIO pins
  gpio_num_t wifi_led_pin;
  gpio_num_t status_pin;
  gpio_num_t boot_state_pin;
  gpio_num_t itho_irq_pin;
  gpio_num_t fail_save_pin;

  // Hardware identification strings
  const char *cve2;
  const char *non_cve1;
  const char *hw_revision;

  HardwareManager();

  /**
   * @brief Detect hardware type and configure GPIO pins accordingly
   *
   * Detects hardware revision by reading GPIO states:
   *  0x3F -> CVE with I2C sniffer capable
   *  0x34 -> NON-CVE with I2C sniffer capable
   *  0x03 -> CVE not I2C sniffer capable
   */
  void detect();

  /**
   * @brief Check if hardware supports I2C sniffer functionality
   * @return true if I2C sniffer is supported, false otherwise
   */
  bool isSnifferCapable() const { return i2c_sniffer_capable; }

  /**
   * @brief Get detected hardware revision
   * @return Hardware revision byte
   */
  uint8_t getRevision() const { return hardware_rev_det; }

  /**
   * @brief Blink the status LED for the given duration, then leave it HIGH.
   * Non-blocking — uses an internal Ticker. Subsequent calls restart the
   * window. Safe to call from boot init.
   */
  void blinkStatusLed(unsigned long durationMs, unsigned long intervalMs = 250);

private:
  Ticker statusLedBlinkTick;
  bool statusLedBlinkState;
  unsigned long statusLedBlinkEndMs;
};

extern HardwareManager hardwareManager;
