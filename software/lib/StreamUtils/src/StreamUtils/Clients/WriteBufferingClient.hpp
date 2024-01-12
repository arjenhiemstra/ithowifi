// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include "../Policies/ConnectForwardingPolicy.hpp"
#include "../Policies/ReadForwardingPolicy.hpp"
#include "../Policies/WriteBufferingPolicy.hpp"
#include "../Ports/DefaultAllocator.hpp"
#include "ClientProxy.hpp"

namespace StreamUtils {

template <typename TAllocator>
struct BasicWriteBufferingClient
    : ClientProxy<ReadForwardingPolicy, WriteBufferingPolicy<TAllocator>,
                  ConnectForwardingPolicy> {
  explicit BasicWriteBufferingClient(Client &target, size_t capacity,
                                     TAllocator allocator = TAllocator())
      : ClientProxy<ReadForwardingPolicy, WriteBufferingPolicy<TAllocator>,
                    ConnectForwardingPolicy>(
            target, ReadForwardingPolicy{},
            WriteBufferingPolicy<TAllocator>{capacity, allocator},
            ConnectForwardingPolicy{}) {}
};

using WriteBufferingClient = BasicWriteBufferingClient<DefaultAllocator>;
}  // namespace StreamUtils