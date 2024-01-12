// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include "../Policies/ConnectForwardingPolicy.hpp"
#include "../Policies/ReadLoggingPolicy.hpp"
#include "../Policies/WriteLoggingPolicy.hpp"
#include "ClientProxy.hpp"

namespace StreamUtils {

struct LoggingClient : ClientProxy<ReadLoggingPolicy, WriteLoggingPolicy,
                                   ConnectForwardingPolicy> {
  LoggingClient(Client &target, Print &log)
      : ClientProxy<ReadLoggingPolicy, WriteLoggingPolicy,
                    ConnectForwardingPolicy>(target, ReadLoggingPolicy{log},
                                             WriteLoggingPolicy{log},
                                             ConnectForwardingPolicy{}) {}
};

}  // namespace StreamUtils