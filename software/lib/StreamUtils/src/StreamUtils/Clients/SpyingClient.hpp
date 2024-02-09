// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include "../Policies/ConnectSpyingPolicy.hpp"
#include "../Policies/ReadSpyingPolicy.hpp"
#include "../Policies/WriteSpyingPolicy.hpp"
#include "ClientProxy.hpp"

namespace StreamUtils {

struct SpyingClient
    : ClientProxy<ReadSpyingPolicy, WriteSpyingPolicy, ConnectSpyingPolicy> {
  SpyingClient(Client &target, Print &log)
      : ClientProxy<ReadSpyingPolicy, WriteSpyingPolicy, ConnectSpyingPolicy>(
            target, {log}, {log}, {log}) {}
};

}  // namespace StreamUtils
