// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include "../Policies/ReadForwardingPolicy.hpp"
#include "../Policies/WriteLoggingPolicy.hpp"
#include "StreamProxy.hpp"

namespace StreamUtils {

struct WriteLoggingStream
    : StreamProxy<ReadForwardingPolicy, WriteLoggingPolicy> {
  WriteLoggingStream(Stream &target, Print &log)
      : StreamProxy<ReadForwardingPolicy, WriteLoggingPolicy>(
            target, ReadForwardingPolicy{}, WriteLoggingPolicy{log}) {}
};

}  // namespace StreamUtils