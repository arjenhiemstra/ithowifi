/*
   Hardware Manager - Implementation
   Part of NRG itho wifi firmware
*/

#include "HardwareManager.h"
#include "../ithodevice/i2c_esp32.h"

// Global instance
HardwareManager hardwareManager;

HardwareManager::HardwareManager()
    : i2c_sniffer_capable(false),
      hardware_rev_det(0),
      wifi_led_pin(GPIO_NUM_17),
      status_pin(GPIO_NUM_16),
      boot_state_pin(GPIO_NUM_0),
      itho_irq_pin(GPIO_NUM_4),
      fail_save_pin(GPIO_NUM_0),
      cve2("2"),
      non_cve1("NON-CVE 1"),
      hw_revision(nullptr)
{
}

void HardwareManager::detect()
{
  delay(1000);

  // Test pin connections to determine which hardware (and revision if applicable) we are dealing with
  pinMode(GPIO_NUM_25, INPUT_PULLDOWN);
  pinMode(GPIO_NUM_26, INPUT_PULLDOWN);
  pinMode(GPIO_NUM_32, INPUT_PULLDOWN);
  pinMode(GPIO_NUM_33, INPUT_PULLDOWN);
  pinMode(GPIO_NUM_21, INPUT_PULLDOWN);
  pinMode(GPIO_NUM_22, INPUT_PULLDOWN);

  delay(50);

  pinMode(GPIO_NUM_25, INPUT);
  pinMode(GPIO_NUM_26, INPUT);
  pinMode(GPIO_NUM_32, INPUT);
  pinMode(GPIO_NUM_33, INPUT);
  pinMode(GPIO_NUM_21, INPUT);
  pinMode(GPIO_NUM_22, INPUT);

  delay(50);

  // Read hardware revision from GPIO pins
  hardware_rev_det = digitalRead(GPIO_NUM_25) << 5 |
                     digitalRead(GPIO_NUM_26) << 4 |
                     digitalRead(GPIO_NUM_32) << 3 |
                     digitalRead(GPIO_NUM_33) << 2 |
                     digitalRead(GPIO_NUM_21) << 1 |
                     digitalRead(GPIO_NUM_22);

  /*
   * Hardware revision mapping:
   * 0x3F -> CVE i2c sniffer capable
   * 0x34 -> NON-CVE i2c sniffer capable
   * 0x03 -> CVE not i2c sniffer capable
   */

  if (hardware_rev_det == 0x3F || hardware_rev_det == 0x34)
  {
    i2c_sniffer_capable = true;

    if (hardware_rev_det == 0x3F) // CVE i2c sniffer capable
    {
      i2cMasterSetPins(GPIO_NUM_26, GPIO_NUM_32);
      i2cSlaveSetPins(GPIO_NUM_26, GPIO_NUM_32);
      i2cSnifferSetPins(GPIO_NUM_21, GPIO_NUM_22);
    }
    else // NON-CVE
    {
      pinMode(GPIO_NUM_27, INPUT);
      pinMode(GPIO_NUM_13, INPUT);
      pinMode(GPIO_NUM_14, INPUT);
      i2cMasterSetPins(GPIO_NUM_27, GPIO_NUM_26);
      i2cSlaveSetPins(GPIO_NUM_27, GPIO_NUM_26);
      i2cSnifferSetPins(GPIO_NUM_14, GPIO_NUM_25);
    }
  }
  else // CVE i2c not sniffer capable
  {
    i2cMasterSetPins(GPIO_NUM_21, GPIO_NUM_22);
    i2cSlaveSetPins(GPIO_NUM_21, GPIO_NUM_22);
  }

  // Configure hardware-specific pins
  if (hardware_rev_det == 0x3F || hardware_rev_det == 0x03) // CVE
  {
    boot_state_pin = GPIO_NUM_27;
    fail_save_pin = GPIO_NUM_14;
    status_pin = GPIO_NUM_13;
    hw_revision = cve2;
  }
  else // NON-CVE
  {
    fail_save_pin = GPIO_NUM_32;
    hw_revision = non_cve1;
  }

  // Configure output pins
  pinMode(wifi_led_pin, OUTPUT);
  digitalWrite(wifi_led_pin, HIGH);

  if (hardware_rev_det == 0x3F || hardware_rev_det == 0x03) // CVE
  {
    pinMode(GPIO_NUM_16, INPUT_PULLUP);
  }

  pinMode(status_pin, OUTPUT);
  digitalWrite(status_pin, LOW);
  pinMode(fail_save_pin, INPUT);
}
