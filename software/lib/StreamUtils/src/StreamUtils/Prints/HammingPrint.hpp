// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include "../Policies/HammingEncodingPolicy.hpp"
#include "../Ports/DefaultAllocator.hpp"
#include "PrintProxy.hpp"

namespace StreamUtils {

template <int N, int K, typename TAllocator>
using BasicHammingPrint = PrintProxy<HammingEncodingPolicy<N, K, TAllocator>>;

template <int N, int K>
using HammingPrint = BasicHammingPrint<N, K, DefaultAllocator>;

}  // namespace StreamUtils