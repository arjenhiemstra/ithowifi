// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#include "SpyingAllocator.hpp"

#include "StreamUtils/Prints/HammingPrint.hpp"
#include "StreamUtils/Prints/SpyingPrint.hpp"
#include "StreamUtils/Prints/StringPrint.hpp"
#include "StreamUtils/Streams/MemoryStream.hpp"

#include "doctest.h"

using namespace StreamUtils;

TEST_CASE("HammingPrint") {
  MemoryStream upstream(4);

  StringPrint log;
  SpyingPrint spy{upstream, log};
  SpyingAllocator allocator{log};

  BasicHammingPrint<7, 4, SpyingAllocator&> print{spy, allocator};

  SUBCASE("write(char*, size_t)") {
    SUBCASE("stops if even byte fails") {
      CHECK(print.print("ABA") == 2);
      print.write('A');  // try to flush remainder

      CHECK(log.str() ==
            "write('TqTbTq', 6) -> 4"
            "write('T') -> 0"  // no remainder
      );
    }

    SUBCASE("saves remainder if odd byte fails") {
      upstream.print("?");

      CHECK(print.print("ABA") == 2);
      print.write('A');  // try to flush remainder

      CHECK(log.str() ==
            "write('TqTbTq', 6) -> 3"
            "write('b') -> 0"  // remainder!!
      );
    }

    SUBCASE("writes remainder first") {
      upstream.print("???");
      print.write('A');  // add remainder
      log.clear();
      upstream.readString();

      CHECK(print.print("ABA") == 2);

      CHECK(log.str() ==
            "write('q') -> 1"
            "write('TqTbTq', 6) -> 3");
    }

    SUBCASE("stops if remainder fails") {
      upstream.print("???");
      print.write('A');  // add remainder
      log.clear();

      CHECK(print.print("ABA") == 0);

      CHECK(log.str() == "write('q') -> 0");
    }

    SUBCASE("allocates in stack when buffer is small") {
      print.write("ABABABABABABABAB", 16);

      CHECK(log.str() == "write('TqTbTqTbTqTbTqTbTqTbTqTbTqTbTqTb', 32) -> 4");
    }

    SUBCASE("allocates in heap when buffer is large") {
      print.write("ABABABABABABABABA", 17);

      CHECK(log.str() ==
            "allocate(34) -> ptr"
            "write('TqTbTqTbTqTbTqTbTqTbTqTbTqTbTqTbTq', 34) -> 4"
            "deallocate(ptr)");
    }

    SUBCASE("falls back to stack if heap fails") {
      allocator.forceFail = true;
      print.write("ABABABABABABABABA", 17);

      CHECK(log.str() ==
            "allocate(34) -> null"
            "write('TqTbTqTbTqTbTqTbTqTbTqTbTqTbTqTb', 32) -> 4");
    }
  }

  SUBCASE("write(char)") {
    SUBCASE("writes both bytes") {
      CHECK(print.write('A') == 1);

      CHECK(upstream.readString() == "Tq");
      CHECK(log.str() ==
            "write('T') -> 1"
            "write('q') -> 1");
    }

    SUBCASE("stops if first by fails") {
      upstream.print("????");

      CHECK(print.write('A') == 0);
      print.write('A');  // try to flush remainder

      CHECK(log.str() ==
            "write('T') -> 0"
            "write('T') -> 0"  // no remainder
      );
    }

    SUBCASE("saves remainder if second by fails") {
      upstream.print("???");

      CHECK(print.write('A') == 1);
      print.write('A');  // try to flush remainder

      CHECK(log.str() ==
            "write('T') -> 1"
            "write('q') -> 0"
            "write('q') -> 0"  // remainder!!
      );
    }

    SUBCASE("writes remainder first") {
      upstream.print("???");
      print.write('A');  // add remainder
      log.clear();
      upstream.readString();

      CHECK(print.write('A') == 1);

      CHECK(log.str() ==
            "write('q') -> 1"
            "write('T') -> 1"
            "write('q') -> 1");
    }

    SUBCASE("stops if remainder fails") {
      upstream.print("???");
      print.write('A');  // add remainder
      log.clear();

      CHECK(print.write('A') == 0);
      print.write('A');  // try to flush remainder

      CHECK(log.str() ==
            "write('q') -> 0"
            "write('q') -> 0"  // remainder!!
      );
    }

    SUBCASE("resets remainder") {
      upstream.print("???");
      print.write('A');  // add remainder
      log.clear();
      upstream.read();

      CHECK(print.write('A') == 0);
      print.write('A');  // try to flush remainder

      CHECK(log.str() ==
            "write('q') -> 1"
            "write('T') -> 0"
            "write('T') -> 0"  // no remainder
      );
    }
  }

#if STREAMUTILS_PRINT_FLUSH_EXISTS
  SUBCASE("flush() writes remainder") {
    upstream.print("???");
    print.write('A');  // add remainder
    log.clear();
    upstream.read();

    print.flush();

    CHECK(log.str() ==
          "write('q') -> 1"
          "flush()"  // no remainder
    );
  }
#endif

  SUBCASE("destructor flushes remainder") {
    {
      BasicHammingPrint<7, 4, SpyingAllocator&> print{spy, allocator};
      upstream.print("???");
      print.write('A');  // add remainder
      log.clear();
    }

    CHECK(log.str() == "write('q') -> 0");
  }
}
