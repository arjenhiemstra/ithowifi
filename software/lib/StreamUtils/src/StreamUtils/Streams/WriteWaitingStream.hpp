// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include "../Policies/ReadForwardingPolicy.hpp"
#include "../Policies/WriteWaitingPolicy.hpp"
#include "StreamProxy.hpp"

namespace StreamUtils {

struct WriteWaitingStream
    : StreamProxy<ReadForwardingPolicy, WriteWaitingPolicy> {
  WriteWaitingStream(Stream &target, Polyfills::function wait = yield)
      : StreamProxy<ReadForwardingPolicy, WriteWaitingPolicy>(
            target, ReadForwardingPolicy{},
            WriteWaitingPolicy{Polyfills::move(wait)}) {}

  void setTimeout(unsigned long timeout) {
    Stream::setTimeout(timeout);
    _writer.setTimeout(timeout);
  }
};

}  // namespace StreamUtils