// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include "../Policies/ReadSpyingPolicy.hpp"
#include "../Policies/WriteSpyingPolicy.hpp"
#include "StreamProxy.hpp"

namespace StreamUtils {

struct SpyingStream : StreamProxy<ReadSpyingPolicy, WriteSpyingPolicy> {
  SpyingStream(Stream &target, Print &log)
      : StreamProxy<ReadSpyingPolicy, WriteSpyingPolicy>(
            target, ReadSpyingPolicy{log}, WriteSpyingPolicy{log}) {}
};

}  // namespace StreamUtils
