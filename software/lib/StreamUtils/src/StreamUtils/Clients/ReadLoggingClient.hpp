// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include "../Policies/ConnectForwardingPolicy.hpp"
#include "../Policies/ReadLoggingPolicy.hpp"
#include "../Policies/WriteForwardingPolicy.hpp"
#include "ClientProxy.hpp"

namespace StreamUtils {

struct ReadLoggingClient : ClientProxy<ReadLoggingPolicy, WriteForwardingPolicy,
                                       ConnectForwardingPolicy> {
  ReadLoggingClient(Client &target, Print &log)
      : ClientProxy<ReadLoggingPolicy, WriteForwardingPolicy,
                    ConnectForwardingPolicy>(target, ReadLoggingPolicy{log},
                                             WriteForwardingPolicy{},
                                             ConnectForwardingPolicy{}) {}
};

}  // namespace StreamUtils