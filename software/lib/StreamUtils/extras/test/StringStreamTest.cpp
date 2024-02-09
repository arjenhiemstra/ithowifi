// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#include "StreamUtils/Streams/StringStream.hpp"

#include "doctest.h"

using namespace StreamUtils;

TEST_CASE("StringStream") {
  WHEN("Constructed with no argument") {
    StringStream s;

    THEN("str() return an empty String") {
      CHECK(s.str() == "");
    }

    THEN("write(uint8_t) appends the string") {
      s.write('A');
      s.write('B');
      CHECK(s.str() == "AB");
    }

    THEN("write(uint8_t) return 1") {
      CHECK(s.write('A') == 1);
      CHECK(s.write('B') == 1);
    }

    THEN("write(0) return 0") {
      CHECK(s.write(0) == 0);
    }

    THEN("write(uint8_t*, size_t) appends the string") {
      s.write(reinterpret_cast<const uint8_t*>("ABXXX"), 2);
      s.write(reinterpret_cast<const uint8_t*>("CDEXX"), 3);
      CHECK(s.str() == "ABCDE");
    }

    THEN("write(uint8_t*, size_t) return number of chars written") {
      uint8_t dummy[32] = {1, 2, 3, 0, 5, 6};
      CHECK(s.write(dummy, 2) == 2);
      CHECK(s.write(dummy, 3) == 3);
      CHECK(s.write(dummy, 4) == 3);
    }

    THEN("str(String) sets the string") {
      s.str("world!");
      CHECK(s.str() == "world!");
    }

    THEN("available() returns 0") {
      CHECK(s.available() == 0);
    }

    THEN("peek() return -1") {
      CHECK(s.peek() == -1);
    }

    THEN("peek() return -1") {
      CHECK(s.read() == -1);
    }
  }

  WHEN("Constructed with string") {
    StringStream s("hello");

    THEN("str() returns the string passed to contructor") {
      CHECK(s.str() == "hello");
    }

    THEN("available() returns the length passed to the constructor") {
      CHECK(s.available() == 5);
    }

    THEN("write(uint8_t) appends the string") {
      s.write('A');
      s.write('B');
      CHECK(s.str() == "helloAB");
    }

    THEN("write(uint8_t*, size_t) appends the string") {
      s.write(reinterpret_cast<const uint8_t*>("ABXXX"), 2);
      s.write(reinterpret_cast<const uint8_t*>("CDEXX"), 3);
      CHECK(s.str() == "helloABCDE");
    }

    THEN("str(String) replaces the string") {
      s.str("world!");
      CHECK(s.str() == "world!");
    }

    THEN("peek() return the first char") {
      CHECK(s.peek() == 'h');
    }

    THEN("peek() return the next char") {
      CHECK(s.read() == 'h');
      CHECK(s.read() == 'e');
    }

    THEN("readBytes() can return the begining of the string") {
      char buffer[32] = {0};
      CHECK(s.readBytes(buffer, 3) == 3);
      CHECK(buffer == std::string("hel"));
      CHECK(s.available() == 2);
    }

    THEN("readBytes() can return the whole string") {
      char buffer[32] = {0};
      CHECK(s.readBytes(buffer, 32) == 5);
      CHECK(buffer == std::string("hello"));
      CHECK(s.available() == 0);
    }
  }
}
