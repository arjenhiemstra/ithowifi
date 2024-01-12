// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

namespace StreamUtils {

class ArduinoThrottler {
 public:
  ArduinoThrottler(uint32_t rate) : _interval(1000000 / rate), _last(0) {}

  void throttle() {
    auto now = micros();
    auto elapsed = now - _last;

    if (elapsed < _interval) {
      delayMicroseconds(_interval - elapsed);
    }

    _last = now;
  }

 private:
  unsigned long _interval;
  unsigned long _last;
};

}  // namespace StreamUtils