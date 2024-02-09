// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include <Stream.h>

#include "../Buffers/CircularBuffer.hpp"
#include "../Configuration.hpp"
#include "../Ports/DefaultAllocator.hpp"

namespace StreamUtils {

template <typename TAllocator>
class BasicMemoryStream : public Stream {
 public:
  BasicMemoryStream(size_t capacity, TAllocator allocator = TAllocator())
      : _buffer(capacity, allocator) {}

  BasicMemoryStream(const BasicMemoryStream &src) : _buffer(src._buffer) {}

  int available() override {
    return static_cast<int>(_buffer.available());
  }

  int peek() override {
    return _buffer.isEmpty() ? -1 : _buffer.peek();
  }

  int read() override {
    return _buffer.isEmpty() ? -1 : _buffer.read();
  }

#if STREAMUTILS_STREAM_READBYTES_IS_VIRTUAL
  size_t readBytes(char *data, size_t size) override {
    return _buffer.readBytes(data, size);
  }
#endif

  size_t write(uint8_t data) override {
    return _buffer.isFull() ? 0 : _buffer.write(data);
  }

#if STREAMUTILS_PRINT_WRITE_VOID_UINT32
  size_t write(const void *p, uint32 size) override {
    const uint8_t *data = reinterpret_cast<const uint8_t *>(p);
#else
  size_t write(const uint8_t *data, size_t size) override {
#endif
    return _buffer.write(data, size);
  }

  using Stream::write;

  void flush() override {
    _buffer.clear();
  }

 private:
  CircularBuffer<TAllocator> _buffer;
};

using MemoryStream = BasicMemoryStream<DefaultAllocator>;

}  // namespace StreamUtils