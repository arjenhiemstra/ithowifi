#pragma once
// Itho/Klimaatgarant RF kWh meters (RAMSES device type 29 = first id byte 0xA4..0xA7, 10E0 "VMS-02J52").
// A meter pushes its counters to the bound WPU as RP 2401, 13 bytes: 00 + three big-endian uint32 Wh
// counters. Single-phase meters fill only the first; the heat pump meter fills all three (per phase,
// unverified). No request is on air, so the add-on can only listen. Captured 2026-11-17/18.
// Dependency-free so the native unit tests can include it directly.

#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace rfkwh
{

constexpr uint8_t RAMSES_TYPE = 0x29; // top 6 bits of the first id byte
constexpr uint16_t OPCODE = 0x2401;
constexpr size_t PAYLOAD_LEN = 13;
constexpr int COUNTERS = 3;
constexpr int MAX_METERS = 8;

inline bool isMeterId(uint8_t id0) { return (id0 >> 2) == RAMSES_TYPE; }

// Ids are shown as the three raw bytes in hex, like everywhere else in the interface: "A7,FC,4D"
// (ramses_rf would print the same meter as 29:261197). out needs 9 bytes.
inline void formatId(const uint8_t id[3], char *out, size_t outLen)
{
  snprintf(out, outLen, "%02X,%02X,%02X", id[0], id[1], id[2]);
}

// Topic-safe variant "A7_FC_4D", same as the rfstatus default source name
inline void formatTopicId(const uint8_t id[3], char *out, size_t outLen)
{
  snprintf(out, outLen, "%02X_%02X_%02X", id[0], id[1], id[2]);
}

struct Reading
{
  uint32_t wh[COUNTERS]{};
};

inline bool decode2401(const uint8_t *p, size_t len, Reading &out)
{
  if (p == nullptr || len != PAYLOAD_LEN || p[0] != 0x00)
    return false;
  for (int i = 0; i < COUNTERS; i++)
  {
    const uint8_t *q = p + 1 + 4 * i;
    out.wh[i] = (static_cast<uint32_t>(q[0]) << 24) | (static_cast<uint32_t>(q[1]) << 16) | (static_cast<uint32_t>(q[2]) << 8) | q[3];
  }
  return true;
}

struct Meter
{
  uint8_t id[3]{};
  Reading reading;
  uint32_t lastSeen{0}; // epoch seconds, 0 = never
  uint32_t frames{0};
  bool used{false};
};

// Fixed table of meters heard since boot, first come first served.
class Store
{
public:
  int find(const uint8_t id[3]) const
  {
    for (int i = 0; i < MAX_METERS; i++)
      if (m_[i].used && m_[i].id[0] == id[0] && m_[i].id[1] == id[1] && m_[i].id[2] == id[2])
        return i;
    return -1;
  }

  // Returns the slot, -1 when the table is full. isNew is set on the first frame of a meter.
  int update(const uint8_t id[3], const Reading &r, uint32_t now, bool &isNew)
  {
    isNew = false;
    int i = find(id);
    if (i < 0)
    {
      for (int j = 0; j < MAX_METERS; j++)
        if (!m_[j].used)
        {
          i = j;
          break;
        }
      if (i < 0)
        return -1;
      m_[i] = Meter{};
      m_[i].used = true;
      m_[i].id[0] = id[0];
      m_[i].id[1] = id[1];
      m_[i].id[2] = id[2];
      isNew = true;
    }
    m_[i].reading = r;
    m_[i].lastSeen = now;
    m_[i].frames++;
    return i;
  }

  int count() const
  {
    int n = 0;
    for (int i = 0; i < MAX_METERS; i++)
      n += m_[i].used ? 1 : 0;
    return n;
  }

  const Meter &at(int i) const { return m_[i]; }
  void clear() { *this = Store{}; }

private:
  Meter m_[MAX_METERS];
};

} // namespace rfkwh
