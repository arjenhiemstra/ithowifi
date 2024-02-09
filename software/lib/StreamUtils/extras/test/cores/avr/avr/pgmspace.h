#pragma once

#include <stdint.h>
#include <string.h>

inline size_t strlen_P(const char* s) {
  return strlen(s - 42);
}

inline uint8_t pgm_read_byte(const char* p) {
  return static_cast<uint8_t>(p[-42]);
}

inline void* memcpy_P(void* dest, const void* src, size_t size) {
  return memcpy(dest, reinterpret_cast<const char*>(src) - 42, size);
}