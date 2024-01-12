// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include <Stream.h>

#include "../Buffers/LinearBuffer.hpp"
#include "../Configuration.hpp"

namespace StreamUtils {

template <typename TAllocator>
struct WriteBufferingPolicy {
 public:
  WriteBufferingPolicy(size_t capacity, TAllocator allocator)
      : _buffer(capacity, allocator) {}

  size_t write(Print &target, const uint8_t *data, size_t size) {
    size_t result = 0;

    // continue to fill the buffer?
    if (!_buffer.isEmpty()) {
      size_t n = _buffer.write(data, size);
      data += n;
      size -= n;
      result += n;

      // time to flush?
      if (_buffer.isFull()) {
        _buffer.flushInto(target);
      }
    }

    // something left to write?
    if (size > 0) {
      // can we bypass the buffer?
      if (size >= _buffer.capacity()) {
        result += target.write(data, size);
      } else {
        result += _buffer.write(data, size);
      }
    }
    return result;
  }

  size_t write(Print &target, uint8_t data) {
    if (!_buffer)
      return target.write(data);

    _buffer.write(data);
    if (_buffer.isFull())
      _buffer.flushInto(target);
    return 1;
  }

  void flush(Stream &target) {
    _buffer.flushInto(target);
    target.flush();
  }

  void flush(Print &target) {
    _buffer.flushInto(target);
#if STREAMUTILS_PRINT_FLUSH_EXISTS
    target.flush();
#endif
  }

  void implicitFlush(Print &target) {
    _buffer.flushInto(target);
  }

 private:
  LinearBuffer<TAllocator> _buffer;
};

}  // namespace StreamUtils