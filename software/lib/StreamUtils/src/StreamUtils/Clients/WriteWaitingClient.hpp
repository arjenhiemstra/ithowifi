// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include "../Policies/ConnectForwardingPolicy.hpp"
#include "../Policies/ReadForwardingPolicy.hpp"
#include "../Policies/WriteWaitingPolicy.hpp"
#include "ClientProxy.hpp"

namespace StreamUtils {

struct WriteWaitingClient
    : ClientProxy<ReadForwardingPolicy, WriteWaitingPolicy,
                  ConnectForwardingPolicy> {
  WriteWaitingClient(Client &target, Polyfills::function wait = yield)
      : ClientProxy<ReadForwardingPolicy, WriteWaitingPolicy,
                    ConnectForwardingPolicy>(
            target, ReadForwardingPolicy{},
            WriteWaitingPolicy{Polyfills::move(wait)},
            ConnectForwardingPolicy{}) {}

  void setTimeout(unsigned long timeout) {
    Client::setTimeout(timeout);
    _writer.setTimeout(timeout);
  }
};

}  // namespace StreamUtils