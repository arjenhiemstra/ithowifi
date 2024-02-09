#include "EEPROM.h"

#include <string.h>  // memcpy

EEPROMClass EEPROM;
static uint8_t commitedData[512];
static uint8_t pendingData[512];

uint8_t EEPROMClass::read(int address) {
  return commitedData[address];
}

void EEPROMClass::write(int address, uint8_t value) {
  pendingData[address] = value;
}

void EEPROMClass::commit() {
  memcpy(commitedData, pendingData, 512);
}