// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#include "FailingAllocator.hpp"

#include "StreamUtils/Prints/StringPrint.hpp"
#include "StreamUtils/Streams/MemoryStream.hpp"
#include "StreamUtils/Streams/SpyingStream.hpp"
#include "StreamUtils/Streams/WriteWaitingStream.hpp"

#include "doctest.h"

using namespace StreamUtils;

TEST_CASE("WriteWaitingStream") {
  MemoryStream upstream(4);

  StringPrint log;
  SpyingStream spy{upstream, log};
  WriteWaitingStream stream{spy, [&spy]() { spy.flush(); }};

  SUBCASE("write(char*, size_t)") {
    SUBCASE("no need to wait") {
      CHECK(stream.print("ABC") == 3);

      CHECK(log.str() == "write('ABC', 3) -> 3");
    }

    SUBCASE("need to wait") {
      CHECK(stream.print("ABCDEFG") == 7);

      CHECK(log.str() ==
            "write('ABCDEFG', 7) -> 4"
            "flush()"
            "write('EFG', 3) -> 3");
    }

    SUBCASE("need to wait twice") {
      CHECK(stream.print("ABCDEFGIJKL") == 11);

      CHECK(log.str() ==
            "write('ABCDEFGIJKL', 11) -> 4"
            "flush()"
            "write('EFGIJKL', 7) -> 4"
            "flush()"
            "write('JKL', 3) -> 3");
    }

    SUBCASE("doesn't wait when timeout is 0") {
      stream.setTimeout(0);

      CHECK(stream.print("ABCDEFG") == 4);

      CHECK(log.str() == "write('ABCDEFG', 7) -> 4");
    }
  }

  SUBCASE("write(char)") {
    SUBCASE("no need to wait") {
      CHECK(stream.write('A') == 1);
      CHECK(stream.write('B') == 1);
      CHECK(stream.write('C') == 1);

      CHECK(log.str() ==
            "write('A') -> 1"
            "write('B') -> 1"
            "write('C') -> 1");
    }

    SUBCASE("need to wait") {
      for (int i = 0; i < 7; i++)
        CHECK(stream.write("ABCDEFG"[i]) == 1);

      CHECK(log.str() ==
            "write('A') -> 1"
            "write('B') -> 1"
            "write('C') -> 1"
            "write('D') -> 1"
            "write('E') -> 0"
            "flush()"
            "write('E') -> 1"
            "write('F') -> 1"
            "write('G') -> 1");
    }

    SUBCASE("doesn't wait when timeout is 0") {
      stream.setTimeout(0);

      CHECK(stream.write('A') == 1);
      CHECK(stream.write('B') == 1);
      CHECK(stream.write('C') == 1);
      CHECK(stream.write('D') == 1);
      CHECK(stream.write('E') == 0);

      CHECK(log.str() ==
            "write('A') -> 1"
            "write('B') -> 1"
            "write('C') -> 1"
            "write('D') -> 1"
            "write('E') -> 0");
    }
  }
}
