// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include <Print.h>

#include "../Configuration.hpp"
#include "../Polyfills.hpp"

namespace StreamUtils {

template <typename WritePolicy>
class PrintProxy : public Print {
 public:
  explicit PrintProxy(Print &upstream, WritePolicy writer = WritePolicy{})
      : _target(upstream), _writer(Polyfills::move(writer)) {}

  PrintProxy(const PrintProxy &other)
      : _target(other._target), _writer(other._writer) {}

  ~PrintProxy() {
    _writer.implicitFlush(_target);
  }

#if STREAMUTILS_PRINT_WRITE_VOID_UINT32
  size_t write(const void *data, uint32 size) override {
    const uint8_t *buffer = reinterpret_cast<const uint8_t *>(data);
#else
  size_t write(const uint8_t *buffer, size_t size) override {
#endif
    return _writer.write(_target, buffer, size);
  }

  size_t write(uint8_t data) override {
    return _writer.write(_target, data);
  }

#if STREAMUTILS_PRINT_FLUSH_EXISTS
  void flush() override {
#else
  void flush() {
#endif
    _writer.flush(_target);
  }

  using Print::write;

 protected:
  Print &_target;
  WritePolicy _writer;
};

}  // namespace StreamUtils