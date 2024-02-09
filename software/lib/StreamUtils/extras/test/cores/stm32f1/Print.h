// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include <WString.h>

#include <stdint.h>
#include <cstring>

typedef uint32_t uint32;

struct Print {
  virtual size_t write(const void *buf, uint32 len) = 0;
  virtual size_t write(uint8_t data) = 0;

  size_t print(const String &s) {
    return write(s.c_str(), s.length());
  }

  size_t print(const char *s) {
    return write(s, std::strlen(s));
  }

  size_t println() {
    return 0;
  }

  template <typename T>
  size_t print(const T &value) {
    return print(String(value));
  }

  template <typename T>
  size_t println(const T &value) {
    return print(value);
  }
};
