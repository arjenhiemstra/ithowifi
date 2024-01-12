// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#if defined(WIN32) || defined(__WIN32) || defined(__WIN32__)
#include <malloc.h>
#else
#include <alloca.h>
#endif

#include "../Configuration.hpp"

namespace StreamUtils {

template <int N, int K, typename TAllocator>
class HammingEncodingPolicy;

template <typename TAllocator>
class HammingEncodingPolicy<7, 4, TAllocator> {
  const static size_t sizeAllowedOnStack = STREAMUTILS_STACK_BUFFER_MAX_SIZE;

 public:
  HammingEncodingPolicy(TAllocator allocator = TAllocator())
      : _allocator(allocator) {}

  size_t write(Print &target, const uint8_t *data, size_t size) {
    if (!flushRemainder(target))
      return 0;

    size_t bufferSize = size * 2;
    uint8_t *buffer;
    if (bufferSize > sizeAllowedOnStack) {
      buffer = static_cast<uint8_t *>(_allocator.allocate(bufferSize));
      if (!buffer) {
        bufferSize = sizeAllowedOnStack;
        buffer = static_cast<uint8_t *>(alloca(bufferSize));
      }
    } else {
      buffer = static_cast<uint8_t *>(alloca(bufferSize));
    }

    for (size_t i = 0, j = 0; j < bufferSize; i++) {
      buffer[j++] = encode(data[i] >> 4);
      buffer[j++] = encode(data[i] & 0x0f);
    }
    size_t n = target.write(buffer, bufferSize);
    if (n & 1) {
      _remainder = buffer[n];
      ++n;
    }

    if (bufferSize > sizeAllowedOnStack)
      _allocator.deallocate(buffer);

    return n / 2;
  }

  size_t write(Print &target, uint8_t data) {
    uint8_t first = encode(data >> 4);
    uint8_t second = encode(data & 0x0f);

    if (!flushRemainder(target))
      return 0;

    if (!target.write(first))
      return 0;

    if (!target.write(second))
      _remainder = second;

    return 1;
  }

  void flush(Stream &target) {
    flushRemainder(target);
    target.flush();
  }

  void flush(Print &target) {
    flushRemainder(target);
#if STREAMUTILS_PRINT_FLUSH_EXISTS
    target.flush();
#endif
  }

  void implicitFlush(Print &target) {
    flushRemainder(target);
  }

 private:
  // Encode 4-bits to 7-bits using Hamming(7,4)
  uint8_t encode(uint8_t input) {
    static uint8_t table[] = {0x00, 0x71, 0x62, 0x13, 0x54, 0x25, 0x36, 0x47,
                              0x38, 0x49, 0x5A, 0x2B, 0x6C, 0x1D, 0x0E, 0x7F};
    return table[input];
  }

  template <typename TTarget>
  bool flushRemainder(TTarget &target) {
    if (_remainder < 0)
      return true;

    if (!target.write(_remainder))
      return false;

    _remainder = -1;
    return true;
  }

  TAllocator _allocator;
  int8_t _remainder = -1;
};

}  // namespace StreamUtils