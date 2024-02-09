// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#include "StreamUtils/Prints/StringPrint.hpp"

#include "doctest.h"

using namespace StreamUtils;

TEST_CASE("StringPrint") {
  WHEN("Constructed with no argument") {
    StringPrint s;

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
  }

  WHEN("Constructed with string") {
    StringPrint s("hello");

    THEN("str() return the stirng passed to contructor") {
      CHECK(s.str() == "hello");
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
  }
}
