// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include "../Policies/ReadBufferingPolicy.hpp"
#include "../Policies/WriteForwardingPolicy.hpp"
#include "../Ports/DefaultAllocator.hpp"
#include "StreamProxy.hpp"

namespace StreamUtils {

template <typename TAllocator>
class BasicReadBufferingStream
    : public StreamProxy<ReadBufferingPolicy<TAllocator>,
                         WriteForwardingPolicy> {
  using base_type =
      StreamProxy<ReadBufferingPolicy<TAllocator>, WriteForwardingPolicy>;

 public:
  explicit BasicReadBufferingStream(Stream &upstream, size_t capacity,
                                    TAllocator allocator = TAllocator())
      : base_type(upstream, {capacity, allocator}, {}) {}
};

using ReadBufferingStream = BasicReadBufferingStream<DefaultAllocator>;
}  // namespace StreamUtils