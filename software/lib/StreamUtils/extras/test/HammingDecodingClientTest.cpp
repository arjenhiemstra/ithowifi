// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#include "SpyingAllocator.hpp"

#include "StreamUtils/Clients/HammingDecodingClient.hpp"
#include "StreamUtils/Clients/MemoryClient.hpp"
#include "StreamUtils/Clients/SpyingClient.hpp"
#include "StreamUtils/Prints/StringPrint.hpp"

#include "doctest.h"

using namespace StreamUtils;

TEST_CASE("HammingDecodingClient") {
  MemoryClient upstream(64);
  StringPrint log;
  SpyingClient spy{upstream, log};
  SpyingAllocator allocator{log};

  BasicHammingDecodingClient<7, 4, SpyingAllocator&> client{spy, allocator};

  SUBCASE("read() with small buffer") {
    uint8_t buffer[8];

    SUBCASE("empty input") {
      size_t result = client.read(buffer, sizeof(buffer));

      CHECK(result == 0);
#if STREAMUTILS_STREAM_READBYTES_IS_VIRTUAL
      CHECK(log.str() == "read(16) -> 0 [timeout]");
#endif
    }

    SUBCASE("decodes input") {
      upstream.print("TqTb");

      size_t result = client.read(buffer, sizeof(buffer));

      CHECK(result == 2);
      CHECK(std::string((char*)buffer, result) == "AB");
#if STREAMUTILS_STREAM_READBYTES_IS_VIRTUAL
      CHECK(log.str() == "read(16) -> 4 [timeout]");
#endif
    }

    SUBCASE("includes byte loaded by peek()") {
      upstream.print("TqTb");
      client.peek();
      log.clear();

      size_t result = client.read(buffer, sizeof(buffer));

      CHECK(result == 2);
      CHECK(std::string((char*)buffer, result) == "AB");
#if STREAMUTILS_STREAM_READBYTES_IS_VIRTUAL
      CHECK(log.str() == "read(15) -> 3 [timeout]");
#endif
    }

    SUBCASE("stores dangling byte") {
      upstream.print("TqT");

      size_t result = client.read(buffer, sizeof(buffer));

      CHECK(result == 1);
      CHECK(std::string((char*)buffer, result) == "A");
#if STREAMUTILS_STREAM_READBYTES_IS_VIRTUAL
      CHECK(log.str() == "read(16) -> 3 [timeout]");
#endif

      log.clear();
      upstream.print("b");
      CHECK(client.peek() == 'B');
      CHECK(log.str() == "peek() -> 98");
    }

    SUBCASE("clears the byte loaded by peek") {
      upstream.print("TqTb");
      client.peek();
      client.read(buffer, sizeof(buffer));
      client.peek();
      CHECK(log.str() ==
            "read() -> 84"
            "peek() -> 113"
            "read(15) -> 3 [timeout]"
            "read() -> -1");
    }
  }

  SUBCASE("read() with large buffer") {
    uint8_t buffer[32];

    SUBCASE("empty input") {
      size_t result = client.read(buffer, sizeof(buffer));

      CHECK(result == 0);
#if STREAMUTILS_STREAM_READBYTES_IS_VIRTUAL
      CHECK(log.str() ==
            "allocate(64) -> ptr"
            "read(64) -> 0 [timeout]"
            "deallocate(ptr)");
#endif
    }

    SUBCASE("decodes input") {
      upstream.print("TqTb");

      size_t result = client.read(buffer, sizeof(buffer));

      CHECK(result == 2);
      CHECK(std::string((char*)buffer, result) == "AB");
#if STREAMUTILS_STREAM_READBYTES_IS_VIRTUAL
      CHECK(log.str() ==
            "allocate(64) -> ptr"
            "read(64) -> 4 [timeout]"
            "deallocate(ptr)");
#endif
    }

    SUBCASE("uses input buffer if allocation fails") {
      upstream.print("TqTb");
      allocator.forceFail = true;

      size_t result = client.read(buffer, sizeof(buffer));

      CHECK(result == 2);
      CHECK(std::string((char*)buffer, result) == "AB");
#if STREAMUTILS_STREAM_READBYTES_IS_VIRTUAL
      CHECK(log.str() ==
            "allocate(64) -> null"
            "read(32) -> 4 [timeout]");
#endif
    }
  }
}
