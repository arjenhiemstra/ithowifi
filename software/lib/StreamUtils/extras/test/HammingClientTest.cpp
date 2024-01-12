// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#include "SpyingAllocator.hpp"

#include "StreamUtils/Clients/HammingClient.hpp"
#include "StreamUtils/Clients/MemoryClient.hpp"

#include "doctest.h"

using namespace StreamUtils;

TEST_CASE("HammingClient") {
  MemoryClient upstream(64);

  HammingClient<7, 4> client{upstream};

  SUBCASE("read() decodes") {
    upstream.print("Tq");

    CHECK(client.read() == 'A');
  }

  SUBCASE("write() encodes") {
    client.write('A');

    CHECK(upstream.readString() == "Tq");
  }
}
