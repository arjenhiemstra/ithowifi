// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#include "StreamUtils/Streams/EepromStream.hpp"

#if STREAMUTILS_ENABLE_EEPROM

#include "doctest.h"

using namespace StreamUtils;

TEST_CASE("EepromStream") {
  SUBCASE("available()") {
    EepromStream s(42, 84);
    CHECK(s.available() == 84);
  }

  SUBCASE("write(uint8_t)") {
    EepromStream s(0, 2);
    CHECK(s.write('a') == 1);
    CHECK(s.write('b') == 1);
    CHECK(s.write('c') == 0);
    CHECK(s.write('d') == 0);
    s.flush();
    CHECK(s.readString() == "ab");
  }

  SUBCASE("write(const uint8_t *, size_t)") {
    EepromStream s(0, 5);
    CHECK(s.write("abc", 3) == 3);
    CHECK(s.write("def", 3) == 2);
    CHECK(s.write("ghi", 3) == 0);
    s.flush();
    CHECK(s.readString() == "abcde");
  }

  SUBCASE("read()") {
    EepromStream s(0, 2);
    s.write("ab", 2);
    CHECK(s.read() == 'a');
    CHECK(s.read() == 'b');
    CHECK(s.read() == -1);
  }

  SUBCASE("peek()") {
    EepromStream s(0, 2);
    s.write("ab", 2);
    CHECK(s.peek() == 'a');
    CHECK(s.peek() == 'a');
    s.read();
    CHECK(s.peek() == 'b');
    s.read();
    CHECK(s.peek() == -1);
  }
}
#endif