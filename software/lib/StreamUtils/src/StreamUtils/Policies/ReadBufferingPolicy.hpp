// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include <Stream.h>

#include "../Buffers/LinearBuffer.hpp"
#include "../Helpers.hpp"

namespace StreamUtils {

template <typename TAllocator>
struct ReadBufferingPolicy {
  ReadBufferingPolicy(size_t capacity, TAllocator allocator = TAllocator())
      : _buffer(capacity, allocator) {}

  ReadBufferingPolicy(const ReadBufferingPolicy &other)
      : _buffer(other._buffer) {}

  int available(Stream &stream) {
    return static_cast<int>(stream.available() + _buffer.available());
  }

  template <typename TTarget>  // Stream or Client
  int read(TTarget &target) {
    if (!_buffer)
      return target.read();

    if (_buffer.available() > 0)
      return _buffer.read();

    size_t avail = static_cast<size_t>(target.available());
    if (avail <= 1)
      return target.read();

    _buffer.reloadFrom(target, avail);
    return _buffer.read();
  }

  int peek(Stream &stream) {
    return isEmpty() ? stream.peek() : _buffer.peek();
  }

  size_t readBytes(Stream &stream, char *buffer, size_t size) {
    return doReadBytes(stream, buffer, size);
  }

  int read(Client &client, uint8_t *buffer, size_t size) {
    return static_cast<int>(
        doReadBytes(client, reinterpret_cast<char *>(buffer), size));
  }

 private:
  bool isEmpty() const {
    return _buffer.available() == 0;
  }

  LinearBuffer<TAllocator> _buffer;

  template <typename TTarget>  // Stream or Client
  size_t doReadBytes(TTarget &target, char *buffer, size_t size) {
    if (!_buffer)
      return readOrReadBytes(target, buffer, size);

    size_t result = 0;

    // can we read from buffer?
    if (_buffer.available() > 0) {
      size_t bytesRead = _buffer.readBytes(buffer, size);
      result += bytesRead;
      buffer += bytesRead;
      size -= bytesRead;
    }

    // still something to read?
    if (size > 0) {
      // (at this point, the buffer is empty)

      size_t avail = static_cast<size_t>(target.available());

      // should we use the buffer?
      if (avail > size && size < _buffer.capacity()) {
        _buffer.reloadFrom(target, avail);
        size_t bytesRead = _buffer.readBytes(buffer, size);
        result += bytesRead;
      } else {
        // we can bypass the buffer
        result += readOrReadBytes(target, buffer, size);
      }
    }

    return result;
  }
};  // namespace StreamUtils

}  // namespace StreamUtils