// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include <Arduino.h>

#include "StreamUtils/Ports/DefaultAllocator.hpp"

class SpyingAllocator {
 public:
  SpyingAllocator(Print& log) : _log(&log) {}

  bool forceFail = false;

  void* allocate(size_t n) {
    void* ptr = forceFail ? 0 : _allocator.allocate(n);
    _log->print("allocate(");
    _log->print(n);
    _log->print(") -> ");
    _log->print(ptr ? "ptr" : "null");
    return ptr;
  }

  void deallocate(void* ptr) {
    _log->print("deallocate(");
    _log->print(ptr ? "ptr" : "null");
    _log->print(")");
    _allocator.deallocate(ptr);
  }

 private:
  Print* _log;
  StreamUtils::DefaultAllocator _allocator;
};
