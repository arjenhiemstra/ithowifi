// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include <stdlib.h>  // size_t

struct FailingAllocator {
  void* allocate(size_t) {
    return nullptr;
  }
  void deallocate(void*) {}
};
