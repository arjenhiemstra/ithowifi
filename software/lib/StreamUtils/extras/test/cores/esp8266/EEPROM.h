#pragma once

#include <stdint.h>

class EEPROMClass {
 public:
  uint8_t read(int);
  void write(int, uint8_t);
  void commit();
};

extern EEPROMClass EEPROM;
