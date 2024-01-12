// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#include "FailingAllocator.hpp"

#include "StreamUtils/Prints/BufferingPrint.hpp"
#include "StreamUtils/Prints/SpyingPrint.hpp"
#include "StreamUtils/Prints/StringPrint.hpp"

#include "doctest.h"

using namespace StreamUtils;

TEST_CASE("BufferingPrint") {
  StringPrint target;

  StringPrint log;
  SpyingPrint spy{target, log};

  GIVEN("capacity is 4") {
    BufferingPrint bufferingPrint{spy, 4};

    SUBCASE("flush() calls write()") {
      bufferingPrint.write("ABC", 3);
      bufferingPrint.flush();

#if STREAMUTILS_PRINT_FLUSH_EXISTS
      CHECK(log.str() ==
            "write('ABC', 3) -> 3"
            "flush()");
#else
      CHECK(log.str() == "write('ABC', 3) -> 3");
#endif
    }

    GIVEN("the buffer is empty") {
      SUBCASE("write(uint8_t)") {
        size_t n = bufferingPrint.write('A');

        CHECK(n == 1);
        CHECK(log.str() == "");
      }

      SUBCASE("write(uint8_t) should flush") {
        bufferingPrint.write('A');
        bufferingPrint.write('B');
        bufferingPrint.write('C');
        bufferingPrint.write('D');
        bufferingPrint.write('E');

        CHECK(log.str() == "write('ABCD', 4) -> 4");
      }

      SUBCASE("write(char*,3) goes in buffer") {
        size_t n = bufferingPrint.write("ABC", 3);

        CHECK(n == 3);
        CHECK(log.str() == "");
      }

      SUBCASE("write(char*,4) bypasses buffer") {
        size_t n = bufferingPrint.write("ABCD", 4);

        CHECK(n == 4);
        CHECK(log.str() == "write('ABCD', 4) -> 4");
      }
      SUBCASE("write(char*,2) bypasses buffer") {
        size_t n = bufferingPrint.write("ABCD", 4);

        CHECK(n == 4);
        CHECK(log.str() == "write('ABCD', 4) -> 4");
      }
    }

    GIVEN("one byte in the buffer") {
      bufferingPrint.write('A');

      SUBCASE("write(char*,3) goes in buffer and flush") {
        size_t n = bufferingPrint.write("BCD", 3);

        CHECK(n == 3);
        CHECK(log.str() == "write('ABCD', 4) -> 4");
      }

      SUBCASE("write(char*,7) bypasses") {
        size_t n = bufferingPrint.write("BCDEFGH", 7);

        CHECK(n == 7);
        CHECK(log.str() ==
              "write('ABCD', 4) -> 4"
              "write('EFGH', 4) -> 4");
      }
    }
  }

  GIVEN("capacity is 0") {
    BufferingPrint bufferingPrint{spy, 0};

    // SUBCASE("capacity()") {
    //   CHECK(bufferingPrint.capacity() == 0);
    // }

    SUBCASE("write(uint8_t) forwards to target") {
      size_t n = bufferingPrint.write('X');

      CHECK(n == 1);
      CHECK(log.str() == "write('X') -> 1");
    }

    SUBCASE("write(char*,1) forwards to target") {
      size_t n = bufferingPrint.write("A", 1);

      CHECK(n == 1);
      CHECK(log.str() == "write('A', 1) -> 1");
    }

    SUBCASE("flush()") {
      bufferingPrint.flush();

#if STREAMUTILS_PRINT_FLUSH_EXISTS
      CHECK(log.str() == "flush()");
#else
      CHECK(log.str() == "");
#endif
    }
  }

  SUBCASE("Destructor should flush") {
    {
      BufferingPrint bufferingPrint{spy, 10};
      bufferingPrint.write("ABC", 3);
    }

    CHECK(log.str() == "write('ABC', 3) -> 3");
  }
}
