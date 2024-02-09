// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include <Client.h>

namespace StreamUtils {

struct ReadForwardingPolicy {
  int available(Stream &target) {
    return target.available();
  }

  int read(Stream &target) {
    return target.read();
  }

  int peek(Stream &target) {
    return target.peek();
  }

  size_t readBytes(Stream &target, char *buffer, size_t size) {
    return target.readBytes(buffer, size);
  }

  int read(Client &target, uint8_t *buffer, size_t size) {
    return target.read(buffer, size);
  }
};

}  // namespace StreamUtils