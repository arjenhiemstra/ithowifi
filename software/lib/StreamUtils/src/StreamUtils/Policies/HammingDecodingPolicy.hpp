// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#if defined(WIN32) || defined(__WIN32) || defined(__WIN32__)
#include <malloc.h>
#else
#include <alloca.h>
#endif

#include <Client.h>
#include <Stream.h>

#include "../Configuration.hpp"
#include "../Helpers.hpp"

namespace StreamUtils {

template <int N, int K, typename TAllocator>
class HammingDecodingPolicy;

template <typename TAllocator>
class HammingDecodingPolicy<7, 4, TAllocator> {
  const static size_t sizeAllowedOnStack = STREAMUTILS_STACK_BUFFER_MAX_SIZE;

 public:
  HammingDecodingPolicy(TAllocator allocator = TAllocator())
      : _allocator(allocator) {}

  int available(Stream &stream) {
    int n = stream.available();
    if (_remainder >= 0)
      n++;
    return n / 2;
  }

  template <typename TTarget>  // Stream or Client
  int read(TTarget &target) {
    if (_remainder < 0) {
      _remainder = target.read();
      if (_remainder < 0)
        return -1;
    }
    int c = target.read();
    if (c < 0)
      return -1;
    int result = decode(_remainder, c);
    _remainder = -1;
    return result;
  }

  int peek(Stream &stream) {
    if (_remainder < 0) {
      _remainder = stream.read();
      if (_remainder < 0)
        return -1;
    }
    int c = stream.peek();
    if (c < 0)
      return -1;
    return decode(_remainder, c);
  }

  size_t readBytes(Stream &stream, char *buffer, size_t size) {
    return doReadBytes(stream, buffer, size);
  }

  int read(Client &client, uint8_t *buffer, size_t size) {
    return static_cast<int>(
        doReadBytes(client, reinterpret_cast<char *>(buffer), size));
  }

 private:
  // Decode 7-bits to 4-bits using Hamming(7,4)
  uint8_t decode(uint8_t input) {
    // table is packed: each element contains two 4-bits values
    static uint8_t table[] = {
        0x00, 0x03, 0x05, 0xE7, 0x09, 0xEB, 0xED, 0xEE, 0x03, 0x33, 0x4D,
        0x63, 0x8D, 0xA3, 0xDD, 0xED, 0x05, 0x2B, 0x55, 0x65, 0x8B, 0xBB,
        0xC5, 0xEB, 0x81, 0x63, 0x65, 0x66, 0x88, 0x8B, 0x8D, 0x6F, 0x09,
        0x27, 0x47, 0x77, 0x99, 0xA9, 0xC9, 0xE7, 0x41, 0xA3, 0x44, 0x47,
        0xA9, 0xAA, 0x4D, 0xAF, 0x21, 0x22, 0xC5, 0x27, 0xC9, 0x2B, 0xCC,
        0xCF, 0x11, 0x21, 0x41, 0x6F, 0x81, 0xAF, 0xCF, 0xFF};
    uint8_t elem = table[input / 2];
    if (input % 2)
      return elem & 0x0f;

    else
      return elem >> 4;
  }

  uint8_t decode(uint8_t first, uint8_t second) {
    return decode(first) << 4 | decode(second);
  }

  template <typename TTarget>  // Stream or Client
  size_t doReadBytes(TTarget &target, char *output, size_t outputSize) {
    char *buffer;
    size_t bufferSize = outputSize * 2;

    if (bufferSize > sizeAllowedOnStack) {
      buffer = static_cast<char *>(_allocator.allocate(bufferSize));
      if (!buffer) {
        // allocation failed => use the input buffer
        bufferSize = outputSize;
        buffer = output;
      }
    } else {
      buffer = static_cast<char *>(alloca(bufferSize));
    }

    size_t loadedSize = 0;
    if (_remainder >= 0)
      buffer[loadedSize++] = _remainder;

    loadedSize +=
        readOrReadBytes(target, buffer + loadedSize, bufferSize - loadedSize);
    for (size_t i = 0; i < loadedSize / 2; i++)
      output[i] = decode(buffer[2 * i], buffer[2 * i + 1]);

    if (loadedSize % 2)
      _remainder = buffer[loadedSize - 1];
    else
      _remainder = -1;

    if (bufferSize > sizeAllowedOnStack)
      _allocator.deallocate(buffer);

    return loadedSize / 2;
  }

  TAllocator _allocator;
  int _remainder = -1;
};

}  // namespace StreamUtils
