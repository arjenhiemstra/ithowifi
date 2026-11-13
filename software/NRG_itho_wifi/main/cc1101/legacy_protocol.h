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

// ---- Decoding received frames (debug log) ----

constexpr uint16_t CMD_FULL = 0xEFF5; // seen in captures, not sent by the add-on

inline const char *cmdName(uint16_t cmd)
{
  switch (cmd)
  {
  case CMD_LOW:
    return "low";
  case CMD_MEDIUM:
    return "medium";
  case CMD_HIGH:
    return "high";
  case CMD_FULL:
    return "full";
  case CMD_TIMER1:
    return "timer1";
  case CMD_TIMER2:
    return "timer2";
  case CMD_TIMER3:
    return "timer3";
  case CMD_JOIN:
    return "join";
  case CMD_LEAVE:
    return "leave";
  default:
    return "unknown";
  }
}

inline uint8_t chipAt(const uint8_t *buf, size_t chip)
{
  return (buf[chip >> 3] >> (7 - (chip & 7))) & 1;
}

// n (<= 32) chips from `chip` as a number, MSB first
inline uint32_t chipsAt(const uint8_t *buf, size_t chip, uint8_t n)
{
  uint32_t v = 0;
  for (uint8_t i = 0; i < n; i++)
    v = (v << 1) | chipAt(buf, chip + i);
  return v;
}

struct Decoded
{
  bool manchesterOk{false}; // 48 valid Manchester bits
  uint16_t badChip{0};      // first chip pair that is not 10 or 01 (manchesterOk false)
  uint8_t header{0};
  uint16_t id{0};
  uint8_t type{0}; // 1 command, 0 join/leave
  uint16_t cmd{0};
  uint8_t crc{0};
  uint8_t crcCalc{0};
  bool footerOk{false}; // 170,171 after the CRC
  bool hasPrev{false};
  uint8_t prev{0}; // type of the frame sent before this one
};

// `start` is the chip right after the 170,173 sync (the sync's padding chip, Manchester data follows)
inline Decoded decodeFrame(const uint8_t *buf, size_t len, size_t start)
{
  Decoded d;
  const size_t chips = len * 8;
  uint64_t all = 0;
  size_t c = start + 1;
  for (int i = 0; i < 48; i++, c += 2)
  {
    if (c + 1 >= chips || chipAt(buf, c) == chipAt(buf, c + 1))
    {
      d.badChip = c;
      return d;
    }
    all = (all << 1) | chipAt(buf, c);
  }
  d.manchesterOk = true;
  const uint64_t data = all >> 8;
  d.header = data >> 32;
  d.id = (data >> 17) & 0x7FFF;
  d.type = (data >> 16) & 1;
  d.cmd = data & 0xFFFF;
  d.crc = all & 0xFF;
  d.crcCalc = crc(data);
  // footer: 170 from its 2nd chip (the 1st is the CRC's last), 171, then the previous frame type
  if (c + 15 <= chips)
    d.footerOk = chipsAt(buf, c, 7) == (170 & 0x7F) && chipsAt(buf, c + 7, 8) == 171;
  if (c + 23 <= chips)
  {
    d.hasPrev = true;
    d.prev = chipsAt(buf, c + 15, 8);
  }
  return d;
}

// Loose capture (sync on the preamble only): find the end of 170,...,170,173. Returns the chip after
// it, or -1. The preamble alternates 1010..., 173 is 10101101: its "11" breaks the alternation.
inline int findFrameStart(const uint8_t *buf, size_t len)
{
  const size_t chips = len * 8;
  for (size_t c = 1; c + 3 < chips; c++)
  {
    if (chipAt(buf, c) == chipAt(buf, c - 1))
      return (chipAt(buf, c) == 1 && chipsAt(buf, c + 1, 2) == 0x1) ? static_cast<int>(c + 3) : -1;
  }
  return -1;
}

} // namespace legacy
