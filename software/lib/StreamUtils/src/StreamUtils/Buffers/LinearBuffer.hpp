// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include <Client.h>
#include <Stream.h>

#include "../Helpers.hpp"
#include "CharArray.hpp"

namespace StreamUtils {

template <typename TAllocator>
class LinearBuffer {
 public:
  LinearBuffer(size_t capacity, TAllocator allocator = TAllocator())
      : _data(capacity, allocator) {
    _begin = _data ? &_data : nullptr;
    _end = _begin;
  }

  LinearBuffer(const LinearBuffer &src) : _data(src._data) {
    if (_data) {
      memcpy(&_data, src._begin, src.available());
      _begin = &_data;
      _end = &_data + src.available();
    } else {
      _begin = nullptr;
      _end = nullptr;
    }
  }

  size_t available() const {
    return _end - _begin;
  }

  size_t capacity() const {
    return _data.size();
  }

  void clear() {
    _begin = _end = _data;
  }

  bool isEmpty() const {
    return available() == 0;
  }

  bool isFull() const {
    return available() == capacity();
  }

  operator bool() const {
    return _data;
  }

  char peek() const {
    return *_begin;
  }

  char read() {
    return *_begin++;
  }

  size_t readBytes(char *dstPtr, size_t dstSize) {
    size_t srcSize = available();
    size_t n = srcSize < dstSize ? srcSize : dstSize;
    memcpy(dstPtr, _begin, n);
    _begin += n;
    return n;
  }

  size_t write(uint8_t data) {
    assert(!isFull());
    *_end++ = data;
    return 1;
  }

  size_t write(const uint8_t *data, size_t size) {
    size_t roomLeft = capacity() - available();
    if (size > roomLeft)
      size = roomLeft;
    memcpy(_end, data, size);
    _end += size;
    return size;
  }

  template <typename TTarget>  // Stream or Client
  void reloadFrom(TTarget &source, size_t size) {
    if (size > _data.size())
      size = _data.size();
    size_t n = readOrReadBytes(source, &_data, size);
    _begin = &_data;
    _end = &_data + n;
  }

  void flushInto(Print &destination) {
    if (_begin != _end)
      destination.write(_begin, _end - _begin);
    _begin = _end = &_data;
  }

 private:
  CharArray<TAllocator> _data;
  char *_begin;
  char *_end;
};

}  // namespace StreamUtils