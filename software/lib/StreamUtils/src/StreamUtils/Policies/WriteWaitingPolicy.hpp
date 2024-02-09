// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include <Arduino.h>

#include "../Buffers/LinearBuffer.hpp"
#include "../Configuration.hpp"
#include "../Polyfills.hpp"

namespace StreamUtils {

struct WriteWaitingPolicy {
 public:
  WriteWaitingPolicy(Polyfills::function wait)
      : _wait(Polyfills::move(wait)), _timeout(1000) {}

  size_t write(Print &target, const uint8_t *data, size_t size) {
    unsigned long startTime = millis();
    size_t totalWritten = 0;

    for (;;) {
      size_t n = target.write(data, size);
      size -= n;
      data += n;
      totalWritten += n;
      if (size == 0 || millis() - startTime >= _timeout)
        return totalWritten;
      _wait();
    }
  }

  size_t write(Print &target, uint8_t data) {
    unsigned long startTime = millis();

    for (;;) {
      if (target.write(data))
        return 1;
      if (millis() - startTime >= _timeout)
        return 0;
      _wait();
    }
  }

  template <typename TTarget>
  void flush(TTarget &target) {
    target.flush();
  }

  void implicitFlush(Print &) {}

  void setTimeout(unsigned long timeout) {
    _timeout = timeout;
  }

 private:
  Polyfills::function _wait;
  unsigned long _timeout;
};

}  // namespace StreamUtils