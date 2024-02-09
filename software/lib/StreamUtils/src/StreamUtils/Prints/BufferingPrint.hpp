// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include "../Policies/WriteBufferingPolicy.hpp"
#include "../Ports/DefaultAllocator.hpp"
#include "PrintProxy.hpp"

namespace StreamUtils {

template <typename TAllocator>
struct BasicBufferingPrint : PrintProxy<WriteBufferingPolicy<TAllocator>> {
  explicit BasicBufferingPrint(Print &upstream, size_t capacity,
                               TAllocator allocator = TAllocator())
      : PrintProxy<WriteBufferingPolicy<TAllocator>>(upstream,
                                                     {capacity, allocator}) {}
};

using BufferingPrint = BasicBufferingPrint<DefaultAllocator>;
}  // namespace StreamUtils