// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include "../Policies/HammingDecodingPolicy.hpp"
#include "../Policies/HammingEncodingPolicy.hpp"
#include "../Ports/DefaultAllocator.hpp"
#include "StreamProxy.hpp"

namespace StreamUtils {

template <int N, int K, typename TAllocator>
using BasicHammingStream = StreamProxy<HammingDecodingPolicy<N, K, TAllocator>,
                                       HammingEncodingPolicy<N, K, TAllocator>>;

template <int N, int K>
using HammingStream = BasicHammingStream<N, K, DefaultAllocator>;

}  // namespace StreamUtils
