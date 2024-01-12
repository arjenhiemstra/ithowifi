// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include <Stream.h>
#include "../Polyfills.hpp"

namespace StreamUtils {

template <typename ReadPolicy, typename WritePolicy>
class StreamProxy : public Stream {
 public:
  explicit StreamProxy(Stream &upstream, ReadPolicy reader = ReadPolicy{},
                       WritePolicy writer = WritePolicy{})
      : _upstream(upstream),
        _reader(reader),
        _writer(Polyfills::move(writer)) {}

  StreamProxy(const StreamProxy &other)
      : _upstream(other._upstream),
        _reader(other._reader),
        _writer(other._writer) {}

  ~StreamProxy() {
    _writer.implicitFlush(_upstream);
  }

#if STREAMUTILS_PRINT_WRITE_VOID_UINT32
  size_t write(const void *data, uint32 size) override {
    const uint8_t *buffer = reinterpret_cast<const uint8_t *>(data);
#else
  size_t write(const uint8_t *buffer, size_t size) override {
#endif
    return _writer.write(_upstream, buffer, size);
  }

  size_t write(uint8_t data) override {
    return _writer.write(_upstream, data);
  }

  using Stream::write;

  int available() override {
    return _reader.available(_upstream);
  }

  int read() override {
    return _reader.read(_upstream);
  }

  int peek() override {
    return _reader.peek(_upstream);
  }

  void flush() override {
    _writer.flush(_upstream);
  }

#if STREAMUTILS_STREAM_READBYTES_IS_VIRTUAL
  size_t readBytes(char *buffer, size_t size) override {
    return _reader.readBytes(_upstream, buffer, size);
  }
#endif

 protected:
  Stream &_upstream;
  ReadPolicy _reader;
  WritePolicy _writer;
};

}  // namespace StreamUtils