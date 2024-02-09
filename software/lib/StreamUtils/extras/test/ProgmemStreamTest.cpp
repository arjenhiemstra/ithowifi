// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#include "StreamUtils/Streams/ProgmemStream.hpp"

#include "doctest.h"

using namespace StreamUtils;

TEST_CASE("ProgmemStream") {
  GIVEN("constructor was called with {0}") {
    ProgmemStream s{static_cast<const char*>(0)};

    THEN("available() should return 0") {
      CHECK(s.available() == 0);
    }

    THEN("peek() should return -1") {
      CHECK(s.peek() == -1);
    }

    THEN("read() should return -1") {
      CHECK(s.read() == -1);
    }

    THEN("readBytes() should return 0") {
      char buffer[8];
      CHECK(s.readBytes(buffer, 8) == 0);
    }
  }

  GIVEN("constructor was called with {F(\"hello\"))}") {
    ProgmemStream s{F("hello")};

    THEN("available() should return 5") {
      CHECK(s.available() == 5);
    }
  }

  GIVEN("constructor was called with {F(\"hello\"), 5)}") {
    ProgmemStream s(F("hello"), 5);

    THEN("write(char) should return 0") {
      CHECK(s.write('a') == 0);
    }

    THEN("print(const char*) should return 0") {
      CHECK(s.print("abc") == 0);
    }

    THEN("available() should return 5") {
      CHECK(s.available() == 5);
    }

    THEN("peek() should return 'h'") {
      CHECK(s.peek() == 'h');
    }

    WHEN("read() is called") {
      int c = s.read();

      THEN("it sould return 'h'") {
        CHECK(c == 'h');
      }

      THEN("available() should return 4") {
        CHECK(s.available() == 4);
      }

      THEN("peek() should return 'e'") {
        CHECK(s.peek() == 'e');
      }
    }

    WHEN("readBytes(buffer, 4) is called") {
      char buffer[4];
      int n = s.readBytes(buffer, 4);

      THEN("it should return 4") {
        CHECK(n == 4);
      }

      THEN("available() should return 1") {
        CHECK(s.available() == 1);
      }

      THEN("peek() should return 'o'") {
        CHECK(s.peek() == 'o');
      }

      THEN("read() should return 'o'") {
        CHECK(s.read() == 'o');
      }
    }

    WHEN("readBytes(buffer, 8) is called") {
      char buffer[8];
      int n = s.readBytes(buffer, 8);

      THEN("it should return 5") {
        CHECK(n == 5);
      }

      THEN("available() should return 0") {
        CHECK(s.available() == 0);
      }

      THEN("peek() should return -1") {
        CHECK(s.peek() == -1);
      }

      THEN("read() should return -1") {
        CHECK(s.read() == -1);
      }
    }
  }
}