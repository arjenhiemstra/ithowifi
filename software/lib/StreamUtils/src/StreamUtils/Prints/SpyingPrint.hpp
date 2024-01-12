// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include "../Policies/WriteSpyingPolicy.hpp"
#include "PrintProxy.hpp"

namespace StreamUtils {

struct SpyingPrint : PrintProxy<WriteSpyingPolicy> {
  SpyingPrint(Print &target, Print &log)
      : PrintProxy<WriteSpyingPolicy>(target, WriteSpyingPolicy{log}) {}
};

}  // namespace StreamUtils
