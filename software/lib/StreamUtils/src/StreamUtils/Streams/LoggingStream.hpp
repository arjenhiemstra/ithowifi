// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include "../Policies/ReadLoggingPolicy.hpp"
#include "../Policies/WriteLoggingPolicy.hpp"
#include "StreamProxy.hpp"

namespace StreamUtils {

struct LoggingStream : StreamProxy<ReadLoggingPolicy, WriteLoggingPolicy> {
  LoggingStream(Stream& target, Print& log)
      : StreamProxy<ReadLoggingPolicy, WriteLoggingPolicy>(
            target, ReadLoggingPolicy{log}, WriteLoggingPolicy{log}) {}
};

}  // namespace StreamUtils