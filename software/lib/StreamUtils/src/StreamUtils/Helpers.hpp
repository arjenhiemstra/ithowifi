// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

namespace StreamUtils {

inline size_t readOrReadBytes(Stream &stream, char *buffer, size_t size) {
  return stream.readBytes(buffer, size);
}

inline size_t readOrReadBytes(Client &client, char *buffer, size_t size) {
  return client.read(reinterpret_cast<uint8_t *>(buffer), size);
}

}  // namespace StreamUtils