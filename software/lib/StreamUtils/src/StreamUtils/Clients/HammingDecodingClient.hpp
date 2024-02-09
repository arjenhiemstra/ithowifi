// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include "../Policies/ConnectForwardingPolicy.hpp"
#include "../Policies/HammingDecodingPolicy.hpp"
#include "../Policies/WriteForwardingPolicy.hpp"
#include "../Ports/DefaultAllocator.hpp"
#include "ClientProxy.hpp"

namespace StreamUtils {

template <int N, int K, typename TAllocator>
using BasicHammingDecodingClient =
    ClientProxy<HammingDecodingPolicy<N, K, TAllocator>, WriteForwardingPolicy,
                ConnectForwardingPolicy>;

template <int N, int K>
using HammingDecodingClient =
    BasicHammingDecodingClient<N, K, DefaultAllocator>;

}  // namespace StreamUtils
