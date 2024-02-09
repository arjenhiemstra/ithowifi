// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include <assert.h>
#include <stdlib.h>  // size_t
#include <string.h>  // memcpy

namespace StreamUtils {

template <typename TAllocator>
class CharArray {
 public:
  CharArray(size_t size, TAllocator allocator = TAllocator())
      : _allocator(allocator) {
    _data = reinterpret_cast<char *>(_allocator.allocate(size));
    _size = _data ? size : 0;
  }

  CharArray(const CharArray &src) : CharArray(src._size, src._allocator) {
    if (_data != nullptr)
      memcpy(_data, src._data, _size);
  }

  ~CharArray() {
    _allocator.deallocate(_data);
  }

  size_t size() const {
    return _size;
  }

  operator bool() const {
    return _size > 0;
  }

  char *operator&() {
    return _data;
  }

  char &operator[](size_t i) {
    return _data[i];
  }

  char operator[](size_t i) const {
    return _data[i];
  }

 protected:
  TAllocator _allocator;
  char *_data;
  size_t _size;
};

}  // namespace StreamUtils