#include "EEPROM.h"

EEPROMClass EEPROM;
static uint8_t data[512];

uint8_t EEPROMClass::read(int address) {
  return data[address];
}

void EEPROMClass::write(int address, uint8_t value) {
  data[address] = value;
}
