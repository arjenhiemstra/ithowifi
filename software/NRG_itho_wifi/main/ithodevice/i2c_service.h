#pragma once

// Itho I2C messages of the debug page: frame building and reply decoding.
// Pure helpers without Arduino/RTOS dependencies, unit-tested natively (test_native_i2c_service).

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "devices/fault_history.h"

namespace i2csvc
{
  constexpr uint8_t SRC = 0x80; // add-on address on the bus
  constexpr uint8_t TYPE_QUERY = 0x04;
  constexpr uint8_t TYPE_INFORM = 0x05;
  constexpr uint8_t TYPE_WRITE = 0x06;
  constexpr size_t MAX_DATA = 32;

  inline uint8_t checksum(const uint8_t *buf, size_t len)
  {
    uint8_t sum = 0;
    while (len--)
      sum += *buf++;
    return static_cast<uint8_t>(-sum);
  }

  // [dest][src][class hi][class lo][type][len][data...][checksum]; returns the frame length, 0 if it does not fit
  inline size_t buildFrame(uint8_t *out, size_t outLen, uint8_t dest, uint16_t msgClass, uint8_t type, const uint8_t *data, size_t len)
  {
    if (len > MAX_DATA || outLen < len + 7 || (len && !data))
      return 0;
    out[0] = dest;
    out[1] = SRC;
    out[2] = msgClass >> 8;
    out[3] = msgClass & 0xFF;
    out[4] = type;
    out[5] = static_cast<uint8_t>(len);
    for (size_t i = 0; i < len; i++)
      out[6 + i] = data[i];
    out[6 + len] = checksum(out, 6 + len);
    return len + 7;
  }

  // "01 0a ff" or "010aff" -> bytes; false on an odd digit count, a bad character or more than max bytes
  inline bool parseHex(const char *s, uint8_t *out, size_t max, size_t &n)
  {
    n = 0;
    int hi = -1;
    for (; s && *s; s++)
    {
      const char c = *s;
      if (c == ' ' || c == ',' || c == ':')
        continue;
      int v;
      if (c >= '0' && c <= '9')
        v = c - '0';
      else if (c >= 'a' && c <= 'f')
        v = c - 'a' + 10;
      else if (c >= 'A' && c <= 'F')
        v = c - 'A' + 10;
      else
        return false;
      if (hi < 0)
        hi = v;
      else
      {
        if (n >= max)
          return false;
        out[n++] = static_cast<uint8_t>(hi << 4 | v);
        hi = -1;
      }
    }
    return hi < 0;
  }

  // Reply as received by i2cSlaveReceive: [0x80][src][class hi][class lo][type][len][payload...][checksum]
  struct Reply
  {
    bool ok = false;
    uint8_t src = 0;
    uint16_t msgClass = 0;
    uint8_t type = 0;
    const uint8_t *payload = nullptr;
    size_t len = 0;
  };

  inline Reply parseReply(const uint8_t *buf, size_t n)
  {
    Reply r;
    if (!buf || n < 7 || static_cast<size_t>(buf[5]) + 7 > n)
      return r;
    r.src = buf[1];
    r.msgClass = static_cast<uint16_t>(buf[2] << 8 | buf[3]);
    r.type = buf[4];
    r.len = buf[5];
    r.payload = buf + 6;
    r.ok = checksum(buf, r.len + 6) == buf[r.len + 6];
    return r;
  }

  // 90E0/10E0 device type payload: [?][manufacturer group][device group][device id][hw version][fw version]...[name from byte 17]
  inline std::string deviceType(const uint8_t *p, size_t len)
  {
    if (!p || len < 6)
      return "too short";
    char s[96];
    snprintf(s, sizeof(s), "device group %u, device id %u, hw version %u, fw version %u", p[2], p[3], p[4], p[5]);
    std::string out = s;
    if (len > 17)
    {
      std::string name;
      for (size_t i = 17; i < len && p[i]; i++)
        name += (p[i] >= 0x20 && p[i] < 0x7F) ? static_cast<char>(p[i]) : '?';
      if (!name.empty())
        out += ", name \"" + name + "\"";
    }
    return out;
  }

  // 10E1 serial number: big-endian, at most 8 bytes
  inline uint64_t serialNumber(const uint8_t *p, size_t len)
  {
    uint64_t v = 0;
    for (size_t i = 0; i < len && i < 8; i++)
      v = v << 8 | p[i];
    return v;
  }

  // 10E2 article number: ASCII, NUL bytes left out
  inline std::string asciiText(const uint8_t *p, size_t len)
  {
    std::string s;
    for (size_t i = 0; i < len; i++)
      if (p[i])
        s += (p[i] >= 0x20 && p[i] < 0x7F) ? static_cast<char>(p[i]) : '?';
    return s;
  }

  // 4220 heat pump fault flags: first half "reported", second half "active"; bit i of byte j = fault code 8j+i.
  // reported lists the faults that were reported but are no longer active.
  inline bool faultFlags(const uint8_t *p, size_t len, std::vector<uint8_t> &active, std::vector<uint8_t> &reported)
  {
    active.clear();
    reported.clear();
    if (!p || len == 0 || len % 2)
      return false;
    const size_t half = len / 2;
    for (size_t j = 0; j < half; j++)
      for (uint8_t i = 0; i < 8; i++)
      {
        const uint8_t m = 1 << i;
        const uint8_t code = static_cast<uint8_t>(j * 8 + i);
        if (p[half + j] & m)
          active.push_back(code);
        else if (p[j] & m)
          reported.push_back(code);
      }
    return true;
  }

  // Fault texts of a device: the entry with the highest fwFrom <= fw, else its lowest one; nullptr if it has none
  inline const ithoFaultTable *findFaultTable(uint8_t dg, uint8_t id, uint8_t fw, const ithoFaultTable *tables = ithoFaultTables,
                                              size_t n = ithoFaultTablesLength)
  {
    const ithoFaultTable *best = nullptr, *lowest = nullptr;
    for (size_t i = 0; i < n; i++)
    {
      const ithoFaultTable &t = tables[i];
      if (t.deviceGroup != dg || t.deviceID != id)
        continue;
      if (!lowest || t.fwFrom < lowest->fwFrom)
        lowest = &t;
      if (t.fwFrom <= fw && (!best || t.fwFrom > best->fwFrom))
        best = &t;
    }
    return best ? best : lowest;
  }

  inline const char *faultText(const ithoFaultTable *t, uint8_t code)
  {
    for (size_t i = 0; t && i < t->count; i++)
      if (t->texts[i].code == code)
        return t->texts[i].label < ithoFaultLabelsLength ? ithoFaultLabels[t->texts[i].label] : nullptr;
    return nullptr;
  }

  struct FaultEntry
  {
    uint8_t code;
    uint32_t time; // 4205 only, unit unknown
  };

  // 3222 reply: [start][count][code...] (count matches the 3221 buffer size on a CVE-Silent)
  // 4205 reply: [start][count][code, time BE32...] (HRU 250/300, DemandFlow)
  inline bool faultHistory(const uint8_t *p, size_t len, uint16_t msgClass, std::vector<FaultEntry> &out)
  {
    out.clear();
    const size_t step = msgClass == 0x4205 ? 5 : 1;
    if (!p || len < 2 || 2 + static_cast<size_t>(p[1]) * step > len)
      return false;
    for (size_t i = 0; i < p[1]; i++)
    {
      const uint8_t *e = p + 2 + i * step;
      out.push_back({e[0], step == 5 ? static_cast<uint32_t>(e[1]) << 24 | static_cast<uint32_t>(e[2]) << 16 | static_cast<uint32_t>(e[3]) << 8 | e[4] : 0});
    }
    return true;
  }

  // One line per entry in reply order: "1: 10 Communication with RF device"
  inline std::string faultHistoryText(const std::vector<FaultEntry> &entries, const ithoFaultTable *t, bool withTime)
  {
    if (entries.empty())
      return "empty";
    std::string s;
    char line[24];
    for (size_t i = 0; i < entries.size(); i++)
    {
      snprintf(line, sizeof(line), "%u: %u ", static_cast<unsigned>(i + 1), entries[i].code);
      const char *text = faultText(t, entries[i].code);
      s += (i ? "\n" : "") + std::string(line) + (text ? text : "unknown");
      if (withTime)
        s += " (time " + std::to_string(entries[i].time) + ")";
    }
    return s;
  }

  // Heat pump fault texts, index = fault code
  inline const char *const wpuFaultText[]{
      "none",
      "outside sensor",
      "boiler bottom sensor",
      "boiler top sensor",
      "evaporator sensor",
      "suction gas sensor",
      "discharge sensor",
      "liquid sensor",
      "source return sensor",
      "source supply sensor",
      "CH return sensor",
      "CH supply sensor",
      "CH pressure sensor",
      "current transformer 1",
      "current transformer 2",
      "source flow problem",
      "manual operation active",
      "high pressure active",
      "high pressure switch defect",
      "CH circuit pressure too low",
      "CH circuit pressure low",
      "clixon active",
      "phase compressor 1",
      "phase detection",
      "compressor current too low",
      "element current too low",
      "compressor current too high",
      "element current too high",
      "frost",
      "source temp too low",
      "condenser temp too high",
      "discharge gas temp too high",
      "evaporator temp too low",
      "too little refrigerant",
      "CH circuit temp far too high",
      "compressor blocked by MFT or gateway",
      "OpenTherm communication",
      "compressor relay stuck",
      "element relay stuck",
      "external deaeration active",
      "DHW element current too high",
      "DHW element current too low",
      "source on and drain fault",
      "wrong thermostat connected"};
  constexpr size_t WPU_FAULT_COUNT = sizeof(wpuFaultText) / sizeof(wpuFaultText[0]);

  inline std::string faultList(const std::vector<uint8_t> &codes)
  {
    if (codes.empty())
      return "none";
    std::string s;
    for (uint8_t c : codes)
    {
      if (!s.empty())
        s += ", ";
      s += std::to_string(c);
      if (c < WPU_FAULT_COUNT)
        s += std::string(" (") + wpuFaultText[c] + ")";
    }
    return s;
  }
} // namespace i2csvc
