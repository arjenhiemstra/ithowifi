// Native unit tests for the I2C service message helpers (main/ithodevice/i2c_service.h):
// frame building, hex input parsing, reply parsing and the 10E1/10E2/4220 decoders.

#include <unity.h>
#include <cstdint>
#include <cstring>

#include "../../main/ithodevice/i2c_service.h"

void setUp() {}
void tearDown() {}

void test_build_frame_matches_known_query()
{
  // 2400 query as the firmware sends it: 82 80 24 00 04 00 D6
  uint8_t f[16];
  const size_t n = i2csvc::buildFrame(f, sizeof(f), 0x82, 0x2400, i2csvc::TYPE_QUERY, nullptr, 0);
  const uint8_t want[]{0x82, 0x80, 0x24, 0x00, 0x04, 0x00, 0xD6};
  TEST_ASSERT_EQUAL_UINT32(sizeof(want), n);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(want, f, sizeof(want));
}

void test_build_frame_with_data()
{
  // 4E34 write with 01 01
  uint8_t f[16];
  const uint8_t d[]{0x01, 0x01};
  const size_t n = i2csvc::buildFrame(f, sizeof(f), 0x82, 0x4E34, i2csvc::TYPE_WRITE, d, 2);
  TEST_ASSERT_EQUAL_UINT32(9, n);
  TEST_ASSERT_EQUAL_HEX8(0x02, f[5]);
  TEST_ASSERT_EQUAL_HEX8(0x01, f[7]);
  TEST_ASSERT_EQUAL_HEX8(i2csvc::checksum(f, 8), f[8]);
  uint8_t sum = 0;
  for (size_t i = 0; i < n; i++)
    sum += f[i];
  TEST_ASSERT_EQUAL_HEX8(0, sum); // frame including checksum sums to zero
}

void test_build_frame_rejects_too_long()
{
  uint8_t f[64];
  uint8_t d[i2csvc::MAX_DATA + 1]{};
  TEST_ASSERT_EQUAL_UINT32(0, i2csvc::buildFrame(f, sizeof(f), 0x82, 0x2410, 4, d, sizeof(d)));
  TEST_ASSERT_EQUAL_UINT32(0, i2csvc::buildFrame(f, 8, 0x82, 0x2410, 4, d, 4)); // output too small
}

void test_parse_hex()
{
  uint8_t b[8];
  size_t n;
  TEST_ASSERT_TRUE(i2csvc::parseHex("01 0a FF", b, sizeof(b), n));
  TEST_ASSERT_EQUAL_UINT32(3, n);
  TEST_ASSERT_EQUAL_HEX8(0x0A, b[1]);
  TEST_ASSERT_TRUE(i2csvc::parseHex("0100,0003", b, sizeof(b), n));
  TEST_ASSERT_EQUAL_UINT32(4, n);
  TEST_ASSERT_TRUE(i2csvc::parseHex("", b, sizeof(b), n));
  TEST_ASSERT_EQUAL_UINT32(0, n);
  TEST_ASSERT_FALSE(i2csvc::parseHex("0", b, sizeof(b), n));   // odd digit count
  TEST_ASSERT_FALSE(i2csvc::parseHex("0g", b, sizeof(b), n));  // bad character
  TEST_ASSERT_FALSE(i2csvc::parseHex("010203", b, 2, n));      // too many bytes
}

void test_parse_reply()
{
  // reply as i2cSlaveReceive stores it: own address first
  uint8_t r[]{0x80, 0x82, 0x10, 0xE2, 0x01, 0x03, 'A', 'B', 0x00, 0x00};
  r[9] = i2csvc::checksum(r, 9);
  i2csvc::Reply p = i2csvc::parseReply(r, sizeof(r));
  TEST_ASSERT_TRUE(p.ok);
  TEST_ASSERT_EQUAL_HEX8(0x82, p.src);
  TEST_ASSERT_EQUAL_HEX16(0x10E2, p.msgClass);
  TEST_ASSERT_EQUAL_UINT32(3, p.len);
  TEST_ASSERT_EQUAL_STRING("AB", i2csvc::asciiText(p.payload, p.len).c_str());
  r[9] ^= 1;
  TEST_ASSERT_FALSE(i2csvc::parseReply(r, sizeof(r)).ok);
  TEST_ASSERT_NULL(i2csvc::parseReply(r, 6).payload);  // too short
  r[5] = 0x20;                                          // length beyond the buffer
  TEST_ASSERT_NULL(i2csvc::parseReply(r, sizeof(r)).payload);
}

void test_serial_number_big_endian()
{
  const uint8_t s[]{0x00, 0x01, 0x02, 0x03};
  TEST_ASSERT_TRUE(i2csvc::serialNumber(s, sizeof(s)) == 0x010203ULL);
  const uint8_t l[]{1, 2, 3, 4, 5, 6, 7, 8, 9};
  TEST_ASSERT_TRUE(i2csvc::serialNumber(l, sizeof(l)) == 0x0102030405060708ULL); // max 8 bytes
}

void test_device_type()
{
  // 90E0 payload of the CVE-Silent test unit (37 bytes)
  const uint8_t p[]{0x00, 0x01, 0x00, 0x1B, 0x31, 0x19, 0x00, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0E, 0x05, 0x07, 0xE2,
                    'C', 'V', 'E', '-', 'R', 'F', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  TEST_ASSERT_EQUAL_STRING("device group 0, device id 27, hw version 49, fw version 25, name \"CVE-RF\"",
                           i2csvc::deviceType(p, sizeof(p)).c_str());
  TEST_ASSERT_EQUAL_STRING("device group 0, device id 27, hw version 49, fw version 25", i2csvc::deviceType(p, 6).c_str());
  TEST_ASSERT_EQUAL_STRING("too short", i2csvc::deviceType(p, 5).c_str());
}

void test_fault_flags()
{
  // 16 bytes: 8 reported, 8 active. Reported: 17 and 28; active: 17 and 33.
  uint8_t p[16]{};
  p[2] |= 1 << 1; // 17 reported
  p[3] |= 1 << 4; // 28 reported
  p[8 + 2] |= 1 << 1; // 17 active
  p[8 + 4] |= 1 << 1; // 33 active
  std::vector<uint8_t> active, reported;
  TEST_ASSERT_TRUE(i2csvc::faultFlags(p, sizeof(p), active, reported));
  TEST_ASSERT_EQUAL_UINT32(2, active.size());
  TEST_ASSERT_EQUAL_UINT8(17, active[0]);
  TEST_ASSERT_EQUAL_UINT8(33, active[1]);
  TEST_ASSERT_EQUAL_UINT32(1, reported.size()); // 17 is active, so only 28 remains
  TEST_ASSERT_EQUAL_UINT8(28, reported[0]);
  TEST_ASSERT_EQUAL_STRING("28 (frost)", i2csvc::faultList(reported).c_str());
  TEST_ASSERT_FALSE(i2csvc::faultFlags(p, 15, active, reported)); // odd length
}

void test_fault_list_unknown_code()
{
  TEST_ASSERT_EQUAL_STRING("none", i2csvc::faultList({}).c_str());
  TEST_ASSERT_EQUAL_STRING("17 (high pressure active), 99", i2csvc::faultList({17, 99}).c_str());
  TEST_ASSERT_EQUAL_UINT32(44, i2csvc::WPU_FAULT_COUNT);
}

void test_fault_history_3222()
{
  // 3222 reply of the CVE-Silent test unit (fw 25): start 0, 10 entries
  const uint8_t p[]{0x00, 0x0A, 0x0A, 0x00, 0x0A, 0x00, 0x0A, 0x00, 0x0A, 0x00, 0x0A, 0x00};
  std::vector<i2csvc::FaultEntry> e;
  TEST_ASSERT_TRUE(i2csvc::faultHistory(p, sizeof(p), 0x3222, e));
  TEST_ASSERT_EQUAL_UINT32(10, e.size());
  const ithoFaultTable *t = i2csvc::findFaultTable(0x00, 0x1B, 25);
  TEST_ASSERT_NOT_NULL(t);
  const std::string s = i2csvc::faultHistoryText(e, t, false);
  TEST_ASSERT_EQUAL_STRING("1: 10 Communication with RF device\n2: 0 No fault", s.substr(0, s.find("\n3:")).c_str());
  TEST_ASSERT_FALSE(i2csvc::faultHistory(p, 11, 0x3222, e)); // count beyond the payload
  TEST_ASSERT_FALSE(i2csvc::faultHistory(p, 1, 0x3222, e));
  const uint8_t none[]{0x00, 0x00};
  TEST_ASSERT_TRUE(i2csvc::faultHistory(none, 2, 0x3222, e));
  TEST_ASSERT_EQUAL_STRING("empty", i2csvc::faultHistoryText(e, t, false).c_str());
}

void test_fault_history_4205()
{
  // HRU 250/300: start 0, 2 entries of [code][time BE32]; codes are matched to Naam (51 = outside sensor)
  const uint8_t p[]{0x00, 0x02, 0x33, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x05};
  std::vector<i2csvc::FaultEntry> e;
  TEST_ASSERT_TRUE(i2csvc::faultHistory(p, sizeof(p), 0x4205, e));
  TEST_ASSERT_EQUAL_UINT32(2, e.size());
  TEST_ASSERT_EQUAL_UINT32(256, e[0].time);
  const ithoFaultTable *t = i2csvc::findFaultTable(0x07, 0x01, 3);
  TEST_ASSERT_NOT_NULL(t);
  TEST_ASSERT_EQUAL_HEX16(0x4205, t->msgClass);
  TEST_ASSERT_EQUAL_STRING("1: 51 Outside sensor fault (time 256)\n2: 1 Filter needs to be cleaned (time 5)",
                           i2csvc::faultHistoryText(e, t, true).c_str());
  TEST_ASSERT_FALSE(i2csvc::faultHistory(p, 11, 0x4205, e));
}

void test_fault_table_lookup()
{
  // CVE-SilentExtPlus has tables from fw 12, 18, 21 and 25; lower firmware gets the lowest one
  TEST_ASSERT_EQUAL_UINT8(12, i2csvc::findFaultTable(0x00, 0x1D, 5)->fwFrom);
  TEST_ASSERT_EQUAL_UINT8(18, i2csvc::findFaultTable(0x00, 0x1D, 20)->fwFrom);
  TEST_ASSERT_EQUAL_UINT8(25, i2csvc::findFaultTable(0x00, 0x1D, 40)->fwFrom);
  TEST_ASSERT_NULL(i2csvc::findFaultTable(0x00, 0x2B, 1)); // HRU 350: no texts
  TEST_ASSERT_NULL(i2csvc::faultText(i2csvc::findFaultTable(0x00, 0x1B, 25), 200));
}

int main(int argc, char **argv)
{
  UNITY_BEGIN();
  RUN_TEST(test_build_frame_matches_known_query);
  RUN_TEST(test_build_frame_with_data);
  RUN_TEST(test_build_frame_rejects_too_long);
  RUN_TEST(test_parse_hex);
  RUN_TEST(test_parse_reply);
  RUN_TEST(test_serial_number_big_endian);
  RUN_TEST(test_device_type);
  RUN_TEST(test_fault_flags);
  RUN_TEST(test_fault_list_unknown_code);
  RUN_TEST(test_fault_history_3222);
  RUN_TEST(test_fault_history_4205);
  RUN_TEST(test_fault_table_lookup);
  return UNITY_END();
}
