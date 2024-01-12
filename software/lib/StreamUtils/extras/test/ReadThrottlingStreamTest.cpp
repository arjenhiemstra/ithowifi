// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#include "FailingAllocator.hpp"

#include "StreamUtils/Streams/ReadThrottlingStream.hpp"
#include "StreamUtils/Streams/StringStream.hpp"

#include "doctest.h"

#include <sstream>

using namespace StreamUtils;

class SpyingThrottler {
 public:
  SpyingThrottler(uint32_t rate) {
    _log << "C(" << rate << ")";
  }

  SpyingThrottler(const SpyingThrottler& src) {
    _log << src._log.str();
  }

  void throttle() {
    _log << "T";
  }

  std::string log() const {
    return _log.str();
  }

 private:
  std::stringstream _log;
};

using ReadThrottlingStream = BasicReadThrottlingStream<SpyingThrottler>;

TEST_CASE("ReadThrottlingStream") {
  StringStream upstream;

  ReadThrottlingStream throttledStream{upstream, 4};
  Stream& stream = throttledStream;
  const SpyingThrottler& throttler = throttledStream.throttler();

  SUBCASE("available()") {
    upstream.print("ABCD");
    CHECK(stream.available() == 4);
    CHECK(throttler.log() == "C(4)");
  }

  SUBCASE("read()") {
    upstream.print("ABCD");
    int n = stream.read();

    CHECK(n == 'A');
    CHECK(throttler.log() == "C(4)T");
  }

  SUBCASE("readBytes()") {
    upstream.print("ABCD");
    char output[8] = {0};

    SUBCASE("read more than available") {
      size_t n = stream.readBytes(output, sizeof(output));

      CHECK(n == 4);
      CHECK(strcmp("ABCD", output) == 0);
      CHECK(throttler.log() == "C(4)TTTTT");
    }

    SUBCASE("read less than available") {
      size_t n = stream.readBytes(output, 2);

      CHECK(n == 2);
      CHECK(strcmp("AB", output) == 0);
      CHECK(throttler.log() == "C(4)TT");
    }

    SUBCASE("read as many as available") {
      size_t n = stream.readBytes(output, 4);

      CHECK(n == 4);
      CHECK(strcmp("ABCD", output) == 0);
      CHECK(throttler.log() == "C(4)TTTT");
    }
  }
}
