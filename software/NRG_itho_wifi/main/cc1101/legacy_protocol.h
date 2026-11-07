#pragma once

// Pre-RAMSES Itho RF ("message1"), the only protocol of older units like the CVE ECO 2.
// Frame layout from Klusjesman (IthoEcoFanRFT) and xs4free (Itho-library), command codes and CRC
// reverse engineered from captures and verified on a CVE ECO 2.
// Dependency-free so the native unit tests can include it directly.

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>

namespace legacy
{

constexpr uint8_t FRAME_LEN = 19;
constexpr uint8_t REPEATS = 3; // a remote sends each frame 3 times, about 55 ms apart

// Command codes (decoded bits)
constexpr uint16_t CMD_LOW = 0xEFFD;
constexpr uint16_t CMD_MEDIUM = 0xEFF8;
constexpr uint16_t CMD_HIGH = 0xEFF4;
constexpr uint16_t CMD_TIMER1 = 0xDDEA;
constexpr uint16_t CMD_TIMER2 = 0xDDD7;
constexpr uint16_t CMD_TIMER3 = 0xDDC3;
constexpr uint16_t CMD_JOIN = 0x01FE;
constexpr uint16_t CMD_LEAVE = 0x03FD;

// Last byte: type of the frame sent before this one
constexpr uint8_t PREV_COMMAND = 85;
constexpr uint8_t PREV_JOIN = 77;
constexpr uint8_t PREV_LEAVE = 82;

inline bool isJoinLeave(uint16_t cmd)
{
  return cmd == CMD_JOIN || cmd == CMD_LEAVE;
}

inline uint8_t prevCode(uint16_t cmd)
{
  return cmd == CMD_JOIN ? PREV_JOIN : (cmd == CMD_LEAVE ? PREV_LEAVE : PREV_COMMAND);
}

// CRC-8 over the 40 data bits: poly 0x08, init 0, MSB first, xorout 0xEE (the lowest bit is always 0)
inline uint8_t crc(uint64_t data40)
{
  uint8_t r = 0;
  for (int i = 39; i >= 0; i--)
  {
    const uint8_t top = r >> 7;
    r <<= 1;
    if (top ^ ((data40 >> i) & 1))
      r ^= 0x08;
  }
  return r ^ 0xEE;
}

// 170,170,170,173, then from the 33rd chip Manchester data (1 = 10, 0 = 01): header 0x5D, 15-bit id,
// type bit (1 command, 0 join/leave), 16-bit command, CRC. Then 170,171 (its first chip is the CRC's
// last) and the previous frame type.
inline void buildFrame(uint16_t id, uint16_t cmd, uint8_t prev, uint8_t out[FRAME_LEN])
{
  const uint64_t data = (0x5DULL << 32) | (static_cast<uint64_t>(id & 0x7FFF) << 17) |
                        (static_cast<uint64_t>(isJoinLeave(cmd) ? 0 : 1) << 16) | cmd;
  const uint64_t all = (data << 8) | crc(data);

  memset(out, 0, FRAME_LEN);
  size_t pos = 0;
  auto put = [&](uint8_t bit)
  {
    if (bit)
      out[pos >> 3] |= 0x80 >> (pos & 7);
    pos++;
  };
  auto putByte = [&](uint8_t b, int fromBit)
  {
    for (int i = fromBit; i >= 0; i--)
      put((b >> i) & 1);
  };

  for (uint8_t b : {170, 170, 170, 173})
    putByte(b, 7);
  put(0); // second chip of the last sync pair
  for (int i = 47; i >= 0; i--)
  {
    const uint8_t bit = (all >> i) & 1;
    put(bit);
    put(!bit);
  }
  putByte(170, 6);
  putByte(171, 7);
  putByte(prev, 7);
}

} // namespace legacy
