// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include "../Policies/WriteWaitingPolicy.hpp"
#include "PrintProxy.hpp"

namespace StreamUtils {

struct WaitingPrint : PrintProxy<WriteWaitingPolicy> {
  WaitingPrint(Print &target, Polyfills::function wait = yield)
      : PrintProxy<WriteWaitingPolicy>(
            target, WriteWaitingPolicy{Polyfills::move(wait)}) {}

  void setTimeout(unsigned long timeout) {
    _writer.setTimeout(timeout);
  }
};

}  // namespace StreamUtils