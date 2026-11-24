#pragma once
// Itho/Klimaatgarant RF kWh meters (RAMSES device type 29 = first id byte 0x74..0x77, 10E0 "VMS-02J52").
// A meter answers RQ 2401 (from any sender, within tens of ms) with RP 2401, 13 bytes: 00 + three
// big-endian uint32 Wh counters, and pushes the same to its bound WPU about once an hour. Single-phase
// meters fill only the first counter; the heat pump meter fills all three. Verified on site 2026-11-24.
// Dependency-free so the native unit tests can include it directly.

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace rfkwh
{

constexpr uint8_t RAMSES_TYPE = 29; // decimal, top 6 bits of the first id byte (0x74..0x77)
constexpr uint16_t OPCODE = 0x2401;
constexpr size_t PAYLOAD_LEN = 13;
constexpr int COUNTERS = 3;
constexpr int MAX_METERS = 8;

inline bool isMeterId(uint8_t id0) { return (id0 >> 2) == RAMSES_TYPE; }

// Ids are shown as the three raw bytes in hex, like everywhere else in the interface: "77,FC,4D"
// (ramses_rf would print the same meter as 29:261197). out needs 9 bytes.
inline void formatId(const uint8_t id[3], char *out, size_t outLen)
{
  snprintf(out, outLen, "%02X,%02X,%02X", id[0], id[1], id[2]);
}

// Topic-safe variant "77_FC_4D", same as the rfstatus default source name
inline void formatTopicId(const uint8_t id[3], char *out, size_t outLen)
{
  snprintf(out, outLen, "%02X_%02X_%02X", id[0], id[1], id[2]);
}

// Parses a meter id: "77,FC,4D" (also ':' or no separator) or ramses style "29:261197" (type:decimal).
// Returns false on anything else.
inline bool parseId(const char *s, uint8_t out[3])
{
  if (s == nullptr)
    return false;
  unsigned type = 0, num = 0;
  int consumed = 0;
  if (sscanf(s, "%2u:%6u%n", &type, &num, &consumed) == 2 && s[consumed] == '\0' && type < 64 && num < (1u << 18) && strchr(s, ':') == s + 2)
  {
    const uint32_t v = (type << 18) | num;
    out[0] = v >> 16 & 0xFF;
    out[1] = v >> 8 & 0xFF;
    out[2] = v & 0xFF;
    return true;
  }
  uint8_t v[3]{};
  int n = 0, digits = 0;
  for (; *s; s++)
  {
    if (*s == ',' || *s == ':' || *s == ' ')
      continue;
    if (!isxdigit(static_cast<unsigned char>(*s)) || n >= 3)
      return false;
    const char c = static_cast<char>(toupper(static_cast<unsigned char>(*s)));
    const uint8_t d = c <= '9' ? c - '0' : c - 'A' + 10;
    v[n] = static_cast<uint8_t>((v[n] << 4) | d);
    if (++digits == 2)
    {
      digits = 0;
      n++;
    }
  }
  if (n != 3 || digits != 0)
    return false;
  memcpy(out, v, 3);
  return true;
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
