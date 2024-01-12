// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include <Client.h>

namespace StreamUtils {

class ReadLoggingPolicy {
 public:
  ReadLoggingPolicy(Print &log) : _log(log) {}

  int available(Stream &stream) {
    return stream.available();
  }

  int read(Stream &stream) {
    int result = stream.read();
    if (result >= 0)
      _log.write(result);
    return result;
  }

  int peek(Stream &stream) {
    return stream.peek();
  }

  size_t readBytes(Stream &stream, char *buffer, size_t size) {
    size_t result = stream.readBytes(buffer, size);
    _log.write(buffer, result);
    return result;
  }

  int read(Client &client, uint8_t *buffer, size_t size) {
    int result = client.read(buffer, size);
    _log.write(buffer, result);
    return result;
  }

 private:
  Print &_log;
};

}  // namespace StreamUtils