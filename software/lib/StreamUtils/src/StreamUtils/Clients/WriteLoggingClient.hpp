// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include "../Policies/ConnectForwardingPolicy.hpp"
#include "../Policies/ReadForwardingPolicy.hpp"
#include "../Policies/WriteLoggingPolicy.hpp"
#include "ClientProxy.hpp"

namespace StreamUtils {

struct WriteLoggingClient
    : ClientProxy<ReadForwardingPolicy, WriteLoggingPolicy,
                  ConnectForwardingPolicy> {
  WriteLoggingClient(Client &target, Print &log)
      : ClientProxy<ReadForwardingPolicy, WriteLoggingPolicy,
                    ConnectForwardingPolicy>(target, ReadForwardingPolicy{},
                                             WriteLoggingPolicy{log},
                                             ConnectForwardingPolicy{}) {}
};

}  // namespace StreamUtils