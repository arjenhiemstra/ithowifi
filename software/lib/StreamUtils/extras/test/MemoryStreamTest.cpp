// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#include "StreamUtils/Streams/MemoryStream.hpp"
#include "StreamUtils/Streams/SpyingStream.hpp"

#include "doctest.h"

using namespace StreamUtils;

TEST_CASE("MemoryStream") {
  MemoryStream stream(4);

  WHEN("stream is empty") {
    THEN("available() return 0") {
      CHECK(stream.available() == 0);
    }

    THEN("write(uint8) returns 1") {
      CHECK(stream.write('A') == 1);
    }

    THEN("write(\"ABCD\",4) returns 4") {
      CHECK(stream.write("ABCD", 4) == 4);
    }

    THEN("write(uint8*,8) returns 4") {
      CHECK(stream.write("ABCDEFGH", 8) == 4);
    }

    THEN("read() return -1") {
      CHECK(stream.read() == -1);
    }

    THEN("peek() return -1") {
      CHECK(stream.peek() == -1);
    }
  }

  WHEN("stream is full") {
    stream.print("ABCD");

    THEN("available() return 4") {
      CHECK(stream.available() == 4);
    }

    THEN("write(uint8) returns 0") {
      CHECK(stream.write('A') == 0);
    }

    THEN("write(\"ABCD\",4) returns 0") {
      CHECK(stream.write("ABCD", 4) == 0);
    }

    THEN("read() returns the next value") {
      CHECK(stream.read() == 'A');
      CHECK(stream.read() == 'B');
    }

    THEN("read(uint8_t*,size_t) extracts the next bytes") {
      char data[5] = {0};
      CHECK(stream.readBytes(data, 4) == 4);
      CHECK(data == std::string("ABCD"));
    }

    THEN("peek() returns the first value") {
      CHECK(stream.peek() == 'A');
      CHECK(stream.peek() == 'A');
    }

    SUBCASE("read() makes room for one byte") {
      stream.read();  // make room
      REQUIRE(stream.available() == 3);

      stream.write('E');  // write at the beginning
      REQUIRE(stream.available() == 4);

      char data[5] = {0};
      stream.readBytes(data, 4);
      CHECK(data == std::string("BCDE"));
    }
  }

  WHEN("lower half is filled") {
    stream.print("AB");

    THEN("available() return 2") {
      CHECK(stream.available() == 2);
    }

    THEN("write(uint8) returns 1") {
      CHECK(stream.write('C') == 1);
    }

    THEN("write(\"ABCD\",4) returns 2") {
      CHECK(stream.write("ABCD", 4) == 2);
    }

    THEN("read() returns the next value") {
      CHECK(stream.read() == 'A');
      CHECK(stream.read() == 'B');
    }

    THEN("read(uint8_t*,size_t) extracts the next bytes") {
      char data[5] = {0};
      CHECK(stream.readBytes(data, 4) == 2);
      CHECK(data == std::string("AB"));
    }

    THEN("peek() returns the first value") {
      CHECK(stream.peek() == 'A');
      CHECK(stream.peek() == 'A');
    }
  }
}
