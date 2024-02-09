// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#include "FailingAllocator.hpp"

#include "StreamUtils/Clients/MemoryClient.hpp"
#include "StreamUtils/Clients/ReadLoggingClient.hpp"
#include "StreamUtils/Clients/SpyingClient.hpp"
#include "StreamUtils/Prints/StringPrint.hpp"

#include "doctest.h"

using namespace StreamUtils;

TEST_CASE("ReadLoggingClient") {
  MemoryClient target(4);

  StringPrint log;
  SpyingClient upstreamSpy{target, log};

  StringPrint output;
  ReadLoggingClient loggingClient{upstreamSpy, output};

  SUBCASE("available()") {
    target.print("ABC");

    size_t n = loggingClient.available();

    CHECK(n == 3);
    CHECK(log.str() == "available() -> 3");
    CHECK(output.str() == "");
  }

  SUBCASE("connect(IPAddress)") {
    int n = loggingClient.connect(IPAddress("1.2.3.4"), 80);

    CHECK(n == 1);
    CHECK(log.str() == "connect('1.2.3.4', 80) -> 1");
    CHECK(output.str() == "");
  }

  SUBCASE("connect(const char*)") {
    int n = loggingClient.connect("1.2.3.4", 80);

    CHECK(n == 1);
    CHECK(log.str() == "connect('1.2.3.4', 80) -> 1");
    CHECK(output.str() == "");
  }

  SUBCASE("connected()") {
    uint8_t n = loggingClient.connected();

    CHECK(n == 0);
    CHECK(log.str() == "connected() -> 0");
    CHECK(output.str() == "");
  }

  SUBCASE("stop()") {
    loggingClient.stop();

    CHECK(log.str() == "stop()");
    CHECK(output.str() == "");
  }

  SUBCASE("operator bool()") {
    bool n = loggingClient.operator bool();

    CHECK(n == true);
    CHECK(log.str() == "operator bool() -> true");
    CHECK(output.str() == "");
  }

  SUBCASE("peek()") {
    target.print("ABC");

    int n = loggingClient.peek();

    CHECK(n == 'A');
    CHECK(log.str() == "peek() -> 65");
    CHECK(output.str() == "");
  }

  SUBCASE("read()") {
    target.print("ABC");

    int n = loggingClient.read();

    CHECK(n == 'A');
    CHECK(log.str() == "read() -> 65");
    CHECK(output.str() == "A");
  }

  SUBCASE("read(uint8_t*,size_t)") {
    target.print("ABC");

    uint8_t s[4] = {0};
    size_t n = loggingClient.read(s, 4);

    CHECK(n == 3);
    CHECK(log.str() == "read(4) -> 3 [timeout]");
    CHECK(output.str() == "ABC");
  }

  SUBCASE("readBytes()") {
    target.print("ABC");

    char s[4] = {0};
    size_t n = loggingClient.readBytes(s, 4);

    CHECK(n == 3);
    CHECK(output.str() == "ABC");
#if STREAMUTILS_STREAM_READBYTES_IS_VIRTUAL
    CHECK(log.str() == "readBytes(4) -> 3 [timeout]");
#endif
  }

  SUBCASE("write(char)") {
    size_t n = loggingClient.write('A');

    CHECK(n == 1);
    CHECK(log.str() == "write('A') -> 1");
    CHECK(output.str() == "");
  }

  SUBCASE("write(char*,size_t)") {
    size_t n = loggingClient.write("ABCDEF", 6);

    CHECK(n == 4);
    CHECK(log.str() == "write('ABCDEF', 6) -> 4");
    CHECK(output.str() == "");
  }

  SUBCASE("flush()") {
    loggingClient.flush();

    CHECK(log.str() == "flush()");
  }
}
