// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include "../Configuration.hpp"

#if STREAMUTILS_ENABLE_EEPROM

#include <EEPROM.h>
#include <Stream.h>

namespace StreamUtils {

class EepromStream : public Stream {
 public:
  EepromStream(size_t address, size_t size)
      : _readAddress(address), _writeAddress(address), _end(address + size) {}

  int available() override {
    return static_cast<int>(_end - _readAddress);
  }

  int read() override {
    if (_readAddress >= _end)
      return -1;
    return EEPROM.read(static_cast<int>(_readAddress++));
  }

  int peek() override {
    if (_readAddress >= _end)
      return -1;
    return EEPROM.read(static_cast<int>(_readAddress));
  }

  void flush() override {
#if STREAMUTILS_USE_EEPROM_COMMIT
    EEPROM.commit();
#endif
  }

  using Print::write;

  size_t write(const uint8_t *buffer, size_t size) override {
    size_t remaining = _end - _writeAddress;
    if (size > remaining)
      size = remaining;
    for (size_t i = 0; i < size; i++) {
      int address = static_cast<int>(_writeAddress++);
#if STREAMUTILS_USE_EEPROM_UPDATE
      EEPROM.update(address, buffer[i]);
#else
      EEPROM.write(address, buffer[i]);
#endif
    }
    return size;
  }

  size_t write(uint8_t data) override {
    if (_writeAddress >= _end)
      return 0;
    int address = static_cast<int>(_writeAddress++);
#if STREAMUTILS_USE_EEPROM_UPDATE
    EEPROM.update(address, data);
#else
    EEPROM.write(address, data);
#endif
    return 1;
  }

 private:
  size_t _readAddress, _writeAddress, _end;
};

}  // namespace StreamUtils

#endif
