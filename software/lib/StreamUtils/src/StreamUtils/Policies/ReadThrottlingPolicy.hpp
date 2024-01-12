// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include <Stream.h>

namespace StreamUtils {

template <typename TThrottler>
struct ReadThrottlingPolicy {
  ReadThrottlingPolicy(TThrottler throttler) : _throttler(throttler) {}

  int available(Stream &stream) {
    return stream.available();
  }

  int read(Stream &stream) {
    _throttler.throttle();
    return stream.read();
  }

  int peek(Stream &stream) {
    _throttler.throttle();
    return stream.peek();
  }

  size_t readBytes(Stream &stream, char *buffer, size_t size) {
    for (size_t i = 0; i < size; i++) {
      int c = read(stream);
      if (c < 0)
        return i;
      buffer[i] = c;
    }
    return size;
  }

  const TThrottler &throttler() const {
    return _throttler;
  }

 private:
  TThrottler _throttler;
};

}  // namespace StreamUtils