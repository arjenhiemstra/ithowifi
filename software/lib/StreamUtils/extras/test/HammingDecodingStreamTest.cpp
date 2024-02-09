// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#include "SpyingAllocator.hpp"

#include "StreamUtils/Prints/StringPrint.hpp"
#include "StreamUtils/Streams/HammingDecodingStream.hpp"
#include "StreamUtils/Streams/SpyingStream.hpp"
#include "StreamUtils/Streams/StringStream.hpp"

#include "doctest.h"

using namespace StreamUtils;

TEST_CASE("HammingDecodingStream") {
  StringStream upstream;
  StringPrint log;
  SpyingStream spy{upstream, log};
  SpyingAllocator allocator{log};

  BasicHammingDecodingStream<7, 4, SpyingAllocator&> stream{spy, allocator};

  SUBCASE("available()") {
    SUBCASE("empty input") {
      CHECK(stream.available() == 0);
      CHECK(log.str() == "available() -> 0");
    }

    SUBCASE("even number of bytes") {
      upstream.print("ABCDEFGH");

      CHECK(stream.available() == 4);
      CHECK(log.str() == "available() -> 8");
    }

    SUBCASE("odd number of bytes") {
      upstream.print("ABCDEFGHI");

      CHECK(stream.available() == 4);
      CHECK(log.str() == "available() -> 9");
    }

    SUBCASE("even number of bytes after peek") {
      upstream.print("ABCDEFGH");
      stream.peek();
      log.clear();
      CHECK(stream.available() == 4);
      CHECK(log.str() == "available() -> 7");
    }

    SUBCASE("odd number of bytes after peek") {
      upstream.print("ABCDEFGHI");
      stream.peek();
      log.clear();
      CHECK(stream.available() == 4);
      CHECK(log.str() == "available() -> 8");
    }
  }

  SUBCASE("peek()") {
    SUBCASE("returns -1 when empty") {
      int result = stream.peek();

      CHECK(result == -1);
      CHECK(log.str() == "read() -> -1");
    }

    SUBCASE("returns -1 when only one byte in input") {
      upstream.print("A");

      int result = stream.peek();

      CHECK(result == -1);
      CHECK(log.str() ==
            "read() -> 65"
            "peek() -> -1");
    }

    SUBCASE("returns decoded value") {
      upstream.print("Tq");

      int result = stream.peek();

      CHECK(result == 'A');
      CHECK(log.str() ==
            "read() -> 84"
            "peek() -> 113");
    }

    SUBCASE("doesn't call read() the second time") {
      upstream.print("A");
      stream.peek();
      log.clear();

      int result = stream.peek();

      CHECK(result == -1);
      CHECK(log.str() == "peek() -> -1");
    }
  }

  SUBCASE("read()") {
    SUBCASE("returns -1 if empty") {
      int result = stream.read();

      CHECK(result == -1);
      CHECK(log.str() == "read() -> -1");
    }

    SUBCASE("returns -1 if only one byte in input") {
      upstream.print("A");

      int result = stream.read();

      CHECK(result == -1);
      CHECK(log.str() ==
            "read() -> 65"
            "read() -> -1");
    }

    SUBCASE("returns decoded value if two bytes in input") {
      upstream.print("Tq");

      int result = stream.read();

      CHECK(result == 'A');
      CHECK(log.str() ==
            "read() -> 84"
            "read() -> 113");
    }

    SUBCASE("reuses the byte read by peek()") {
      upstream.print("Tq");
      stream.peek();
      log.clear();

      int result = stream.read();

      CHECK(result == 'A');
      CHECK(log.str() == "read() -> 113");
    }

    SUBCASE("reuses the byte read by previous call") {
      upstream.print("T");
      stream.read();
      upstream.print("q");
      log.clear();

      int result = stream.read();

      CHECK(result == 'A');
      CHECK(log.str() == "read() -> 113");
    }

    SUBCASE("flushes the buffered byte") {
      upstream.print("TqTb");

      CHECK(stream.read() == 'A');
      CHECK(stream.read() == 'B');

      CHECK(log.str() ==
            "read() -> 84"
            "read() -> 113"
            "read() -> 84"
            "read() -> 98");
    }
  }

  SUBCASE("readBytes() with small buffer") {
    char buffer[8];

    SUBCASE("empty input") {
      size_t result = stream.readBytes(buffer, sizeof(buffer));

      CHECK(result == 0);
#if STREAMUTILS_STREAM_READBYTES_IS_VIRTUAL
      CHECK(log.str() == "readBytes(16) -> 0 [timeout]");
#endif
    }

    SUBCASE("decodes input") {
      upstream.print("TqTb");

      size_t result = stream.readBytes(buffer, sizeof(buffer));

      CHECK(result == 2);
      CHECK(std::string(buffer, result) == "AB");
#if STREAMUTILS_STREAM_READBYTES_IS_VIRTUAL
      CHECK(log.str() == "readBytes(16) -> 4 [timeout]");
#endif
    }

    SUBCASE("includes byte loaded by peek()") {
      upstream.print("TqTb");
      stream.peek();
      log.clear();

      size_t result = stream.readBytes(buffer, sizeof(buffer));

      CHECK(result == 2);
      CHECK(std::string(buffer, result) == "AB");
#if STREAMUTILS_STREAM_READBYTES_IS_VIRTUAL
      CHECK(log.str() == "readBytes(15) -> 3 [timeout]");
#endif
    }

    SUBCASE("stores dangling byte") {
      upstream.print("TqT");

      size_t result = stream.readBytes(buffer, sizeof(buffer));

      CHECK(result == 1);
      CHECK(std::string(buffer, result) == "A");
#if STREAMUTILS_STREAM_READBYTES_IS_VIRTUAL
      CHECK(log.str() == "readBytes(16) -> 3 [timeout]");
#endif

      log.clear();
      upstream.print("b");
      CHECK(stream.peek() == 'B');
      CHECK(log.str() == "peek() -> 98");
    }

    SUBCASE("clears the byte loaded by peek") {
      upstream.print("TqTb");
      stream.peek();
      stream.readBytes(buffer, sizeof(buffer));
      CHECK(stream.peek() == -1);
#if STREAMUTILS_STREAM_READBYTES_IS_VIRTUAL
      CHECK(log.str() ==
            "read() -> 84"
            "peek() -> 113"
            "readBytes(15) -> 3 [timeout]"
            "read() -> -1");
#endif
    }
  }

  SUBCASE("readBytes() with large buffer") {
    char buffer[32];

    SUBCASE("empty input") {
      size_t result = stream.readBytes(buffer, sizeof(buffer));

      CHECK(result == 0);
#if STREAMUTILS_STREAM_READBYTES_IS_VIRTUAL
      CHECK(log.str() ==
            "allocate(64) -> ptr"
            "readBytes(64) -> 0 [timeout]"
            "deallocate(ptr)");
#endif
    }

    SUBCASE("decodes input") {
      upstream.print("TqTb");

      size_t result = stream.readBytes(buffer, sizeof(buffer));

      CHECK(result == 2);
      CHECK(std::string(buffer, result) == "AB");
#if STREAMUTILS_STREAM_READBYTES_IS_VIRTUAL
      CHECK(log.str() ==
            "allocate(64) -> ptr"
            "readBytes(64) -> 4 [timeout]"
            "deallocate(ptr)");
#endif
    }

    SUBCASE("uses input buffer if allocation fails") {
      upstream.print("TqTb");
      allocator.forceFail = true;

      size_t result = stream.readBytes(buffer, sizeof(buffer));

      CHECK(result == 2);
      CHECK(std::string(buffer, result) == "AB");
#if STREAMUTILS_STREAM_READBYTES_IS_VIRTUAL
      CHECK(log.str() ==
            "allocate(64) -> null"
            "readBytes(32) -> 4 [timeout]");
#endif
    }
  }

  SUBCASE("flush()") {
    stream.flush();
    CHECK(log.str() == "flush()");
  }
}
