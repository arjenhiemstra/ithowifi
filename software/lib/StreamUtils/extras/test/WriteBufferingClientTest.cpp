// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#include "FailingAllocator.hpp"

#include "StreamUtils/Clients/MemoryClient.hpp"
#include "StreamUtils/Clients/SpyingClient.hpp"
#include "StreamUtils/Clients/WriteBufferingClient.hpp"
#include "StreamUtils/Prints/StringPrint.hpp"

#include "doctest.h"

using namespace StreamUtils;

TEST_CASE("WriteBufferingClient") {
  MemoryClient target(64);

  StringPrint log;
  SpyingClient spy{target, log};

  GIVEN("capacity is 4") {
    WriteBufferingClient bufferingClient{spy, 4};

    SUBCASE("available()") {
      target.print("ABC");

      CHECK(bufferingClient.available() == 3);
      CHECK(log.str() == "available() -> 3");
    }

    SUBCASE("connect(IPAddress)") {
      int n = bufferingClient.connect(IPAddress("1.2.3.4"), 80);

      CHECK(n == 1);
      CHECK(log.str() == "connect('1.2.3.4', 80) -> 1");
    }

    SUBCASE("connect(const char*)") {
      int n = bufferingClient.connect("1.2.3.4", 80);

      CHECK(n == 1);
      CHECK(log.str() == "connect('1.2.3.4', 80) -> 1");
    }

    SUBCASE("connected()") {
      uint8_t n = bufferingClient.connected();

      CHECK(n == 0);
      CHECK(log.str() == "connected() -> 0");
    }

    SUBCASE("stop()") {
      bufferingClient.write("ABC", 3);
      bufferingClient.stop();

      CHECK(log.str() ==
            "write('ABC', 3) -> 3"
            "stop()");
    }

    SUBCASE("operator bool()") {
      bool n = bufferingClient.operator bool();

      CHECK(n == true);
      CHECK(log.str() == "operator bool() -> true");
    }

    SUBCASE("flush() forwards to target)") {
      bufferingClient.flush();

      CHECK(log.str() == "flush()");
    }

    SUBCASE("flush() calls write() and flush()") {
      bufferingClient.write("ABC", 3);
      bufferingClient.flush();

      CHECK(log.str() ==
            "write('ABC', 3) -> 3"
            "flush()");
    }

    SUBCASE("peek()") {
      target.print("ABC");

      CHECK(bufferingClient.peek() == 'A');
      CHECK(log.str() == "peek() -> 65");
    }

    SUBCASE("read()") {
      target.print("ABC");

      CHECK(bufferingClient.read() == 'A');
      CHECK(log.str() == "read() -> 65");
    }

    SUBCASE("readBytes()") {
      target.print("ABC");

      char s[4] = {0};
      size_t n = bufferingClient.readBytes(s, 3);

      CHECK(n == 3);
      CHECK(s == std::string("ABC"));
#if STREAMUTILS_STREAM_READBYTES_IS_VIRTUAL
      CHECK(log.str() == "readBytes(3) -> 3");
#endif
    }

    GIVEN("the buffer is empty") {
      SUBCASE("write(uint8_t)") {
        size_t n = bufferingClient.write('A');

        CHECK(n == 1);
        CHECK(log.str() == "");
      }

      SUBCASE("write(uint8_t) should flush") {
        bufferingClient.write('A');
        bufferingClient.write('B');
        bufferingClient.write('C');
        bufferingClient.write('D');
        bufferingClient.write('E');

        CHECK(log.str() == "write('ABCD', 4) -> 4");
      }

      SUBCASE("write(char*,3) goes in buffer") {
        size_t n = bufferingClient.write("ABC", 3);

        CHECK(n == 3);
        CHECK(log.str() == "");
      }

      SUBCASE("write(char*,4) bypasses buffer") {
        size_t n = bufferingClient.write("ABCD", 4);

        CHECK(n == 4);
        CHECK(log.str() == "write('ABCD', 4) -> 4");
      }
      SUBCASE("write(char*,2) bypasses buffer") {
        size_t n = bufferingClient.write("ABCD", 4);

        CHECK(n == 4);
        CHECK(log.str() == "write('ABCD', 4) -> 4");
      }
    }

    GIVEN("one byte in the buffer") {
      bufferingClient.write('A');

      SUBCASE("write(char*,3) goes in buffer and flush") {
        size_t n = bufferingClient.write("BCD", 3);

        CHECK(n == 3);
        CHECK(log.str() == "write('ABCD', 4) -> 4");
      }

      SUBCASE("write(char*,7) bypasses") {
        size_t n = bufferingClient.write("BCDEFGH", 7);

        CHECK(n == 7);
        CHECK(log.str() ==
              "write('ABCD', 4) -> 4"
              "write('EFGH', 4) -> 4");
      }
    }
  }

  GIVEN("capacity is 0") {
    WriteBufferingClient bufferingClient{spy, 0};

    SUBCASE("write(uint8_t) forwards to target") {
      size_t n = bufferingClient.write('X');

      CHECK(n == 1);
      CHECK(log.str() == "write('X') -> 1");
    }

    SUBCASE("write(char*,1) forwards to target") {
      size_t n = bufferingClient.write("A", 1);

      CHECK(n == 1);
      CHECK(log.str() == "write('A', 1) -> 1");
    }

    SUBCASE("flush() forwards to target") {
      bufferingClient.flush();

      CHECK(log.str() == "flush()");
    }
  }

  SUBCASE("Destructor should flush") {
    {
      WriteBufferingClient bufferingClient{spy, 10};
      bufferingClient.write("ABC", 3);
    }

    CHECK(log.str() == "write('ABC', 3) -> 3");
  }
}
