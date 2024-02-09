// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include "../Policies/ConnectForwardingPolicy.hpp"
#include "../Policies/ReadBufferingPolicy.hpp"
#include "../Policies/WriteForwardingPolicy.hpp"
#include "../Ports/DefaultAllocator.hpp"
#include "ClientProxy.hpp"

namespace StreamUtils {

template <typename TAllocator>
class BasicReadBufferingClient
    : public ClientProxy<ReadBufferingPolicy<TAllocator>, WriteForwardingPolicy,
                         ConnectForwardingPolicy> {
  using base_type = ClientProxy<ReadBufferingPolicy<TAllocator>,
                                WriteForwardingPolicy, ConnectForwardingPolicy>;

 public:
  explicit BasicReadBufferingClient(Client &target, size_t capacity,
                                    TAllocator allocator = TAllocator())
      : base_type(target, ReadBufferingPolicy<TAllocator>{capacity, allocator},
                  WriteForwardingPolicy{}, ConnectForwardingPolicy{}) {}
};

using ReadBufferingClient = BasicReadBufferingClient<DefaultAllocator>;
}  // namespace StreamUtils