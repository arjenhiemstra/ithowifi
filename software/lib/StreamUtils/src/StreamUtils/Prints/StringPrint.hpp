// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include <Print.h>
#include <WString.h>

#include "../Configuration.hpp"
#include "../Polyfills.hpp"

namespace StreamUtils {

class StringPrint : public Print {
 public:
  StringPrint() {}

  explicit StringPrint(String str) : _str(Polyfills::move(str)) {}

#if STREAMUTILS_PRINT_WRITE_VOID_UINT32
  size_t write(const void* data, uint32 n) override {
    const uint8_t* p = reinterpret_cast<const uint8_t*>(data);
#else
  size_t write(const uint8_t* p, size_t n) override {
#endif
    for (size_t i = 0; i < n; i++) {
      uint8_t c = p[i];
      if (c == 0)
        return i;
      write(c);
    }
    return n;
  }

  size_t write(uint8_t c) override {
    if (c == 0)
      return 0;
    _str += static_cast<char>(c);
    return 1;
  }

  const String& str() const {
    return _str;
  }

  void str(String str) {
    _str = Polyfills::move(str);
  }

  void clear() {
    _str = "";
  }

 private:
  String _str;
};
}  // namespace StreamUtils