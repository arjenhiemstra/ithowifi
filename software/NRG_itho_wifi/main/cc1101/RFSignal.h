#pragma once

// Signal diagnostics for received RF packets: pure helpers (no Arduino/RTOS dependency, unit-tested natively).
// CC1101 datasheet (SWRS061): RSSI and FREQEST are two's complement status registers; RSSI is in 0.5 dB steps
// with an offset of 74 dB at 868 MHz / 38.4 kBaud, FREQEST is f_xosc / 2^14 per step (1587 Hz at 26 MHz).

#include <cstdint>

namespace rfsignal
{
  constexpr int RSSI_OFFSET_DB = 74;
  constexpr long XOSC_HZ = 26000000L;

  // Received signal strength in dBm (rounded down to whole dB).
  inline int rssiDbm(uint8_t raw)
  {
    const int v = raw >= 128 ? static_cast<int>(raw) - 256 : static_cast<int>(raw);
    return (v < 0 ? (v - 1) / 2 : v / 2) - RSSI_OFFSET_DB;
  }

  // Frequency offset of the received carrier in Hz (positive = above the programmed frequency).
  inline long freqOffsetHz(uint8_t raw)
  {
    return static_cast<long>(static_cast<int8_t>(raw)) * XOSC_HZ / 16384L;
  }

  // Link quality estimate (lower = better); bit 7 is CRC_OK, which this firmware does not use.
  inline uint8_t linkQuality(uint8_t raw) { return raw & 0x7F; }

  // Cumulative receive counters since boot, logged as one "RFSTAT" line per minute when RF logging is on.
  struct Counters
  {
    uint32_t sync = 0;      // sync words detected (RF interrupts)
    uint32_t coalesced = 0; // interrupts that arrived before the task read the previous packet: most likely lost
    uint32_t decoded = 0;   // packets read and decoded
    uint32_t hdrErr = 0;    // header could not be parsed (noise, collision, foreign protocol)
    uint32_t chkErr = 0;    // checksum mismatch
    uint32_t known = 0;     // from a registered remote/device
    uint32_t knownErr = 0;  // from a registered remote/device but with an error: the ones that matter
  };

  // Account one decoded packet: error is IthoPacket::error (0 ok, 1 header, 2 checksum), known = sender registered.
  inline void count(Counters &c, uint8_t error, bool known)
  {
    c.decoded++;
    if (error == 1)
      c.hdrErr++;
    else if (error == 2)
      c.chkErr++;
    if (known)
    {
      c.known++;
      if (error)
        c.knownErr++;
    }
  }
} // namespace rfsignal
