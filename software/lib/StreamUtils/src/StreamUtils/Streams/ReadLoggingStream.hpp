// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include "../Policies/ReadLoggingPolicy.hpp"
#include "../Policies/WriteForwardingPolicy.hpp"
#include "StreamProxy.hpp"

namespace StreamUtils {

struct ReadLoggingStream
    : StreamProxy<ReadLoggingPolicy, WriteForwardingPolicy> {
  ReadLoggingStream(Stream &target, Print &log)
      : StreamProxy<ReadLoggingPolicy, WriteForwardingPolicy>(
            target, ReadLoggingPolicy{log}, WriteForwardingPolicy{}) {}
};
}  // namespace StreamUtils