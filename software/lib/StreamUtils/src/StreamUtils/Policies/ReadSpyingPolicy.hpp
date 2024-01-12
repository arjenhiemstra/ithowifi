// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include <Client.h>

namespace StreamUtils {

class ReadSpyingPolicy {
 public:
  ReadSpyingPolicy(Print &log) : _log(log) {}

  int available(Stream &target) {
    int result = target.available();
    _log.print("available() -> ");
    _log.println(result);
    return result;
  }

  int read(Stream &target) {
    int result = target.read();
    _log.print("read() -> ");
    _log.println(result);
    return result;
  }

  int peek(Stream &target) {
    int result = target.peek();
    _log.print("peek() -> ");
    _log.println(result);
    return result;
  }

  size_t readBytes(Stream &target, char *buffer, size_t size) {
    size_t result = target.readBytes(buffer, size);
    _log.print("readBytes(");
    _log.print(size);
    _log.print(") -> ");
    _log.print(result);
    if (size > result)
      _log.print(" [timeout]");
    _log.println();
    return result;
  }

  int read(Client &target, uint8_t *buffer, size_t size) {
    int result = target.read(buffer, size);
    _log.print("read(");
    _log.print(size);
    _log.print(") -> ");
    _log.print(result);
    if (static_cast<int>(size) > result)
      _log.print(" [timeout]");
    _log.println();
    return result;
  }

 private:
  Print &_log;
};

}  // namespace StreamUtils
