// StreamUtils - github.com/bblanchon/ArduinoStreamUtils
// Copyright Benoit Blanchon 2019-2023
// MIT License

#pragma once

#include <Client.h>

namespace StreamUtils {

class ConnectSpyingPolicy {
 public:
  ConnectSpyingPolicy(Print& log) : _log(log) {}

  int connect(Client& target, const IPAddress& ip, uint16_t port) {
    _log.print("connect('");
    _log.print(ip);
    _log.print("', ");
    _log.print(port);
    _log.print(") -> ");

    int result = target.connect(ip, port);
    _log.println(result);

    return result;
  }

  int connect(Client& target, const char* ip, uint16_t port) {
    _log.print("connect('");
    _log.print(ip);
    _log.print("', ");
    _log.print(port);
    _log.print(") -> ");

    int result = target.connect(ip, port);
    _log.println(result);

    return result;
  }

  uint8_t connected(Client& target) {
    _log.print("connected() -> ");

    uint8_t result = target.connected();
    _log.println(result);

    return result;
  }

  void stop(Client& target) {
    _log.print("stop()");
    target.stop();
  }

  bool operator_bool(Client& target) {
    _log.print("operator bool() -> ");

    bool result = target.operator bool();
    _log.println(result ? "true" : "false");

    return result;
  }

 private:
  Print& _log;
};

}  // namespace StreamUtils