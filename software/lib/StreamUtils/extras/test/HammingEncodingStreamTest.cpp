// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#include "SpyingAllocator.hpp"

#include "StreamUtils/Streams/HammingEncodingStream.hpp"
#include "StreamUtils/Streams/MemoryStream.hpp"

#include "doctest.h"

using namespace StreamUtils;

TEST_CASE("HammingEncodingStream") {
  MemoryStream upstream(64);

  HammingEncodingStream<7, 4> stream{upstream};

  SUBCASE("read() forwards upstream data") {
    upstream.print("A");

    CHECK(stream.read() == 'A');
  }

  SUBCASE("write() encodes") {
    stream.write('A');

    CHECK(upstream.readString() == "Tq");
  }
}
