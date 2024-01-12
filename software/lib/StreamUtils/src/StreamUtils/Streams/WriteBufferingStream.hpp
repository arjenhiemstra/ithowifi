// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include "../Policies/ReadForwardingPolicy.hpp"
#include "../Policies/WriteBufferingPolicy.hpp"
#include "../Ports/DefaultAllocator.hpp"
#include "StreamProxy.hpp"

namespace StreamUtils {

template <typename TAllocator>
struct BasicWriteBufferingStream
    : StreamProxy<ReadForwardingPolicy, WriteBufferingPolicy<TAllocator>> {
  explicit BasicWriteBufferingStream(Stream &upstream, size_t capacity,
                                     TAllocator allocator = TAllocator())
      : StreamProxy<ReadForwardingPolicy, WriteBufferingPolicy<TAllocator>>(
            upstream, {}, {capacity, allocator}) {}
};

using WriteBufferingStream = BasicWriteBufferingStream<DefaultAllocator>;
}  // namespace StreamUtils