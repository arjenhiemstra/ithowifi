#pragma once

#include <stdint.h>

class EEPROMClass {
 public:
  uint8_t read(int);
  void update(int, uint8_t);
  // void write(int, uint8_t); <- it exists but we want to use update() instead
};

extern EEPROMClass EEPROM;
