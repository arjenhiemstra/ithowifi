// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include "../Policies/ConnectForwardingPolicy.hpp"
#include "../Policies/HammingDecodingPolicy.hpp"
#include "../Policies/HammingEncodingPolicy.hpp"
#include "../Ports/DefaultAllocator.hpp"
#include "ClientProxy.hpp"

namespace StreamUtils {

template <int N, int K, typename TAllocator>
using BasicHammingClient = ClientProxy<HammingDecodingPolicy<N, K, TAllocator>,
                                       HammingEncodingPolicy<N, K, TAllocator>,
                                       ConnectForwardingPolicy>;

template <int N, int K>
using HammingClient = BasicHammingClient<N, K, DefaultAllocator>;

}  // namespace StreamUtils
