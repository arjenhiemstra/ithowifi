// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include "../Policies/HammingEncodingPolicy.hpp"
#include "../Policies/ReadForwardingPolicy.hpp"
#include "../Ports/DefaultAllocator.hpp"
#include "StreamProxy.hpp"

namespace StreamUtils {

template <int N, int K, typename TAllocator>
using BasicHammingEncodingStream =
    StreamProxy<ReadForwardingPolicy, HammingEncodingPolicy<N, K, TAllocator>>;

template <int N, int K>
using HammingEncodingStream =
    BasicHammingEncodingStream<N, K, DefaultAllocator>;

}  // namespace StreamUtils
