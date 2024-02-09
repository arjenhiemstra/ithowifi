// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include "../Policies/WriteLoggingPolicy.hpp"
#include "PrintProxy.hpp"

namespace StreamUtils {

struct LoggingPrint : PrintProxy<WriteLoggingPolicy> {
  LoggingPrint(Print &upstream, Print &log)
      : PrintProxy<WriteLoggingPolicy>(upstream, {log}) {}
};

}  // namespace StreamUtils