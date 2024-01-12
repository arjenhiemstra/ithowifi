// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#include "SpyingAllocator.hpp"

#include "StreamUtils/Clients/HammingEncodingClient.hpp"
#include "StreamUtils/Clients/MemoryClient.hpp"

#include "doctest.h"

using namespace StreamUtils;

TEST_CASE("HammingEncodingClient") {
  MemoryClient upstream(64);

  HammingEncodingClient<7, 4> client{upstream};

  SUBCASE("read() forwards upstream data") {
    upstream.print("A");

    CHECK(client.read() == 'A');
  }

  SUBCASE("write() encodes") {
    client.write('A');

    CHECK(upstream.readString() == "Tq");
  }
}
