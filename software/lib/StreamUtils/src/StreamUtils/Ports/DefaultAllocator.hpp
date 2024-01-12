// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

namespace StreamUtils {

#include <stdlib.h>  // malloc, free, size_t

struct DefaultAllocator {
  void* allocate(size_t n) {
    return malloc(n);
  }

  void deallocate(void* p) {
    free(p);
  }
};

}  // namespace StreamUtils