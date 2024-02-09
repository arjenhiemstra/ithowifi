// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include "../Policies/HammingDecodingPolicy.hpp"
#include "../Policies/WriteForwardingPolicy.hpp"
#include "../Ports/DefaultAllocator.hpp"
#include "StreamProxy.hpp"

namespace StreamUtils {

template <int N, int K, typename TAllocator>
using BasicHammingDecodingStream =
    StreamProxy<HammingDecodingPolicy<N, K, TAllocator>, WriteForwardingPolicy>;

template <int N, int K>
using HammingDecodingStream =
    BasicHammingDecodingStream<N, K, DefaultAllocator>;

}  // namespace StreamUtils
