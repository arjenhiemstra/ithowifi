#pragma once

#include <stdint.h>

class EEPROMClass {
 public:
  uint8_t read(int);
  void write(int, uint8_t);
  bool commit();
};

extern EEPROMClass EEPROM;
