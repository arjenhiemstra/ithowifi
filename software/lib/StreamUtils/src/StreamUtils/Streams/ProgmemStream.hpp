// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include <Arduino.h>

#include "../Configuration.hpp"

namespace StreamUtils {

class ProgmemStream : public Stream {
 public:
  ProgmemStream(const void* ptr, size_t size)
      : _ptr(reinterpret_cast<const char*>(ptr)), _size(size) {}

  ProgmemStream(const char* ptr) : _ptr(ptr), _size(ptr ? strlen_P(ptr) : 0) {}

  ProgmemStream(const __FlashStringHelper* ptr)
      : ProgmemStream{reinterpret_cast<const char*>(ptr)} {}

  int available() override {
    return _size;
  }

  int read() override {
    if (_size <= 0)
      return -1;
    _size--;
    return pgm_read_byte(_ptr++);
  }

#if STREAMUTILS_STREAM_READBYTES_IS_VIRTUAL
  size_t readBytes(char* buffer, size_t size) override {
    if (size > _size)
      size = _size;
    memcpy_P(buffer, _ptr, size);
    _ptr += size;
    _size -= size;
    return size;
  }
#endif

  int peek() override {
    if (_size <= 0)
      return -1;
    return pgm_read_byte(_ptr);
  }

  void flush() override {}

#if STREAMUTILS_PRINT_WRITE_VOID_UINT32
  size_t write(const void*, uint32) override {
    return 0;
  }
#else
  size_t write(const uint8_t*, size_t) override {
    return 0;
  }
#endif

  size_t write(uint8_t) override {
    return 0;
  }

 private:
  const char* _ptr;
  size_t _size;
};

}  // namespace StreamUtils