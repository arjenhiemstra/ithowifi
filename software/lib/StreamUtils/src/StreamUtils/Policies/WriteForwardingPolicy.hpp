// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include <Client.h>
#include <Stream.h>

#include "../Configuration.hpp"

namespace StreamUtils {

struct WriteForwardingPolicy {
  template <typename... Args>
  size_t write(Stream &stream, Args... args) {
    return stream.write(args...);
  }

  void flush(Stream &stream) {
    stream.flush();
  }

  void implicitFlush(Stream &) {}
};

}  // namespace StreamUtils