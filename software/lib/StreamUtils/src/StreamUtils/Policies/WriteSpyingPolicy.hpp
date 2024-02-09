// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include <Stream.h>

#include "../Configuration.hpp"

namespace StreamUtils {

class WriteSpyingPolicy {
 public:
  WriteSpyingPolicy(Print &log) : _log(log) {}

  size_t write(Print &stream, const uint8_t *buffer, size_t size) {
    _log.print("write('");
    for (size_t i = 0; i < size; i++) {
      _log.write(buffer[i]);
    }
    _log.print("', ");
    _log.print(size);
    _log.print(") -> ");

    size_t result = stream.write(buffer, size);
    _log.println(result);

    return result;
  }

  size_t write(Print &stream, uint8_t data) {
    _log.print("write('");
    _log.write(data);
    _log.print("') -> ");

    size_t result = stream.write(data);
    _log.println(result);

    return result;
  }

  template <typename TTarget>
  void flush(TTarget &target) {
    _log.println("flush()");
    target.flush();
  }

  void implicitFlush(Print &) {}

 private:
  Print &_log;
};

}  // namespace StreamUtils
