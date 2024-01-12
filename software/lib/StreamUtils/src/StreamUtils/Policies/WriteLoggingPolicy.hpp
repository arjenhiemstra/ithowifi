// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include <Client.h>
#include "../Configuration.hpp"

namespace StreamUtils {

class WriteLoggingPolicy {
 public:
  WriteLoggingPolicy(Print &log) : _log(log) {}

  size_t write(Print &target, const uint8_t *buffer, size_t size) {
    size_t result = target.write(buffer, size);
    _log.write(buffer, result);
    return result;
  }

  size_t write(Print &target, uint8_t c) {
    size_t result = target.write(c);
    _log.write(c);
    return result;
  }

  template <typename TTarget>
  void flush(TTarget &target) {
    target.flush();
  }

  void implicitFlush(Print &) {}

 private:
  Print &_log;
};

}  // namespace StreamUtils