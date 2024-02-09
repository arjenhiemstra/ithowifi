// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include <stdint.h>

#include "CharArray.hpp"

namespace StreamUtils {

template <typename TAllocator>
class CircularBuffer {
 public:
  CircularBuffer(size_t capacity, TAllocator allocator = TAllocator())
      : _data(capacity, allocator) {
    _begin = _end = _size = 0;
  }

  CircularBuffer(const CircularBuffer &src)
      : CircularBuffer(src._data.size(), src._allocator) {
    if (_data) {
      _begin = src._begin;
      _end = src._end;
      _size = src._size;
    }
  }

  size_t available() const {
    return _size;
  }

  size_t capacity() const {
    return _data.size();
  }

  void clear() {
    _begin = _end = _size = 0;
  }

  bool isEmpty() const {
    return _size == 0;
  }

  bool isFull() const {
    return _size == _data.size();
  }

  char peek() const {
    assert(_size > 0);
    return _data[_begin];
  }

  char read() {
    assert(_size > 0);
    char result = _data[_begin];
    _begin = (_begin + 1) % _data.size();
    _size--;
    return result;
  }

  size_t readBytes(char *data, size_t size) {
    // don't read more that available
    if (size > _size)
      size = _size;

    for (size_t i = 0; i < size; i++)
      data[i] = read();

    return size;
  }

  size_t write(uint8_t data) {
    assert(_size < _data.size());
    _data[_end] = data;
    _end = (_end + 1) % _data.size();
    _size++;
    return 1;
  }

  size_t write(const uint8_t *data, size_t size) {
    // don't read more that available
    size_t roomLeft = _data.size() - _size;
    if (size > roomLeft)
      size = roomLeft;

    for (size_t i = 0; i < size; i++)
      write(data[i]);

    return size;
  }

 private:
  CharArray<TAllocator> _data;
  size_t _size;
  size_t _begin;
  size_t _end;
};

}  // namespace StreamUtils