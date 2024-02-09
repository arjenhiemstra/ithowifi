// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include "../Policies/ConnectForwardingPolicy.hpp"
#include "../Policies/HammingEncodingPolicy.hpp"
#include "../Policies/ReadForwardingPolicy.hpp"
#include "../Ports/DefaultAllocator.hpp"
#include "ClientProxy.hpp"

namespace StreamUtils {

template <int N, int K, typename TAllocator>
using BasicHammingEncodingClient =
    ClientProxy<ReadForwardingPolicy, HammingEncodingPolicy<N, K, TAllocator>,
                ConnectForwardingPolicy>;

template <int N, int K>
using HammingEncodingClient =
    BasicHammingEncodingClient<N, K, DefaultAllocator>;

}  // namespace StreamUtils
