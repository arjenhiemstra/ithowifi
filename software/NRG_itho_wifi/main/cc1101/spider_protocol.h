#pragma once
// Itho Spider thermostat RF payload helpers (opcodes 01FF, 22C9, 1FC9).
// Dependency-free so the native unit tests can include it directly.
// Encodings: 01FF uses 0.5 °C per unit (1 byte), 22C9 uses 0.01 °C (BE16).

#include <cstddef>
#include <cstdint>

namespace spider
{

constexpr uint16_t OPCODE_STATUS = 0x01FF;
constexpr size_t STATUS_LEN = 26;
constexpr uint8_t TEMP_NONE = 0x80; // placeholder in 01FF temperature bytes

constexpr uint8_t FLAG_SCHEDULE = 0x40; // byte 5 bit 6: time program active
constexpr uint8_t FLAG_MANUAL = 0x20;   // byte 5 bit 5: setpoint adjusted on the device

constexpr int32_t SETPOINT_MIN = 500;  // 5.0 °C, in 0.01 °C
constexpr int32_t SETPOINT_MAX = 3000; // 30.0 °C

struct Status
{
  uint8_t roomTemp;     // 0.5 °C units, TEMP_NONE if absent
  uint8_t heatSetpoint; // 0.5 °C units
  uint8_t coolSetpoint; // 0.5 °C units
  uint8_t flags;
};

inline int32_t halfToCenti(uint8_t v) { return static_cast<int32_t>(v) * 50; }
inline uint8_t centiToHalf(int32_t c) { return static_cast<uint8_t>((c + 25) / 50); }

// Clamp to the supported range and round to the 0.5 °C grid the Spider uses.
inline int32_t clampSetpoint(int32_t c)
{
  if (c < SETPOINT_MIN)
    c = SETPOINT_MIN;
  if (c > SETPOINT_MAX)
    c = SETPOINT_MAX;
  return halfToCenti(centiToHalf(c));
}

// 01FF as sent by the Spider (RQ/I): byte 2 room temp, 3 heating setpoint, 4 second setpoint, 5 flags.
inline bool decodeStatus(const uint8_t *p, size_t len, Status &out)
{
  if (p == nullptr || len != STATUS_LEN)
    return false;
  out = Status{p[2], p[3], p[4], p[5]};
  return out.heatSetpoint != TEMP_NONE;
}

// Controller side RP/W 01FF. Fixed template from the community spider.yaml blueprint;
// the Spider's own RQ payload differs (bytes 0, 7, 11-12, 15-16, 23) and is not echoed.
inline void buildControllerStatus(uint8_t out[STATUS_LEN], int32_t setpointCenti)
{
  static const uint8_t tpl[STATUS_LEN] = {0x00, 0x80, 0x80, 0x00, 0x00, 0xD0, 0x00, 0x00, 0x00, 0x14, 0x3C, 0x80, 0x80,
                                          0x00, 0x00, 0xB4, 0x00, 0x80, 0x80, 0x02, 0x80, 0xFF, 0x80, 0x04, 0x00, 0x00};
  for (size_t i = 0; i < STATUS_LEN; i++)
    out[i] = tpl[i];
  const int32_t sp = clampSetpoint(setpointCenti);
  out[3] = centiToHalf(sp);
  out[4] = centiToHalf(sp + 100); // second setpoint: heating + 1 °C, as the blueprint does
}

// 22C9 zone setpoint: [zone][setpoint BE16, 0.01 °C][second setpoint BE16][...]
constexpr size_t ZONE_SETPOINT_ECHO_LEN = 6;

inline bool decodeZoneSetpoint(const uint8_t *p, size_t len, int32_t &centi)
{
  if (p == nullptr || len < 3)
    return false;
  centi = static_cast<int32_t>(p[1]) << 8 | p[2];
  return true;
}

// I 22C9 echo the controller sends after the Spider's W 22C9 (manual dial change).
inline void buildZoneSetpointEcho(uint8_t out[ZONE_SETPOINT_ECHO_LEN], int32_t centi)
{
  const uint16_t v = static_cast<uint16_t>(centi);
  out[0] = 0x00;
  out[1] = v >> 8;
  out[2] = v & 0xFF;
  out[3] = 0x08;
  out[4] = 0x02;
  out[5] = 0x01;
}

// 1FC9 offer entries are [domain][code BE16][device id].
constexpr size_t BIND_ENTRY_LEN = 6;

inline bool offersCode(const uint8_t *p, size_t len, uint16_t code)
{
  if (p == nullptr || len < BIND_ENTRY_LEN || len % BIND_ENTRY_LEN != 0)
    return false;
  for (size_t i = 0; i < len; i += BIND_ENTRY_LEN)
  {
    if (p[i + 1] == (code >> 8) && p[i + 2] == (code & 0xFF))
      return true;
  }
  return false;
}

enum class JoinKind : uint8_t
{
  None,
  Controller, // offers 22C9 (+000E): binds to a 01FF controller (gateway / home icon); accepted with 00 01FF <id>
  Relay       // offers 3150/0008 only: binds as heat demand sender to a heater; use discovery instead
};

inline JoinKind heatingJoinKind(const uint8_t *p, size_t len)
{
  if (offersCode(p, len, 0x22C9))
    return JoinKind::Controller;
  if (offersCode(p, len, 0x3150))
    return JoinKind::Relay;
  return JoinKind::None;
}

// W 1FC9 accept: the controller binds the 01FF channel to its own id.
inline void buildBindAccept(uint8_t out[BIND_ENTRY_LEN], const uint8_t id[3])
{
  out[0] = 0x00;
  out[1] = 0x01;
  out[2] = 0xFF;
  out[3] = id[0];
  out[4] = id[1];
  out[5] = id[2];
}

} // namespace spider
