// Native unit tests for the RF kWh meter helpers (main/cc1101/rf_kwh_meter.h).
// Vectors are real RP 2401 frames from Klimaatgarant meters, captured 2026-11-17/18 with ramses_rf.

#include <unity.h>
#include <cstdint>
#include <cstring>

#include "../../main/cc1101/rf_kwh_meter.h"

using namespace rfkwh;

void setUp() {}
void tearDown() {}

// 77,FC,4D = 29:261197 in ramses_rf (heat pump meter, three counters), 77,FB,A8 = 29:261032 (ventilation unit, one counter)
static const uint8_t ID_WP[3] = {0x77, 0xFC, 0x4D};
static const uint8_t ID_WTW[3] = {0x77, 0xFB, 0xA8};
static const uint8_t ID_WPU[3] = {0x83, 0x91, 0x5D}; // 20:233821, not a meter

// RP 29:261197 -> 20:233821 2401 013 000064E57400431DBD004BD1B0
static const uint8_t RP_WP[13] = {0x00, 0x00, 0x64, 0xE5, 0x74, 0x00, 0x43, 0x1D, 0xBD, 0x00, 0x4B, 0xD1, 0xB0};
// RP 29:261032 -> 20:233821 2401 013 00002B8FB40000000000000000
static const uint8_t RP_WTW[13] = {0x00, 0x00, 0x2B, 0x8F, 0xB4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

void test_decode_three_counters()
{
  Reading r;
  TEST_ASSERT_TRUE(decode2401(RP_WP, sizeof(RP_WP), r));
  TEST_ASSERT_EQUAL_UINT32(6612340, r.wh[0]); // 6612.340 kWh
  TEST_ASSERT_EQUAL_UINT32(4398525, r.wh[1]);
  TEST_ASSERT_EQUAL_UINT32(4968880, r.wh[2]);
}

void test_decode_single_counter()
{
  Reading r;
  TEST_ASSERT_TRUE(decode2401(RP_WTW, sizeof(RP_WTW), r));
  TEST_ASSERT_EQUAL_UINT32(2854836, r.wh[0]);
  TEST_ASSERT_EQUAL_UINT32(0, r.wh[1]);
  TEST_ASSERT_EQUAL_UINT32(0, r.wh[2]);
}

void test_decode_rejects_bad_input()
{
  Reading r;
  TEST_ASSERT_FALSE(decode2401(RP_WP, 12, r));
  TEST_ASSERT_FALSE(decode2401(RP_WP, 14, r));
  TEST_ASSERT_FALSE(decode2401(nullptr, 13, r));
  uint8_t bad[13];
  memcpy(bad, RP_WP, 13);
  bad[0] = 0x01; // index byte is always 00 in the captures
  TEST_ASSERT_FALSE(decode2401(bad, 13, r));
}

void test_parse_id(void)
{
  uint8_t id[3];
  TEST_ASSERT_TRUE(rfkwh::parseId("77,FC,4D", id));
  TEST_ASSERT_EQUAL_HEX8(0x77, id[0]);
  TEST_ASSERT_EQUAL_HEX8(0xFC, id[1]);
  TEST_ASSERT_EQUAL_HEX8(0x4D, id[2]);
  TEST_ASSERT_TRUE(rfkwh::parseId("77:fc:4d", id));
  TEST_ASSERT_EQUAL_HEX8(0x4D, id[2]);
  TEST_ASSERT_TRUE(rfkwh::parseId("77FC4D", id));
  TEST_ASSERT_EQUAL_HEX8(0xFC, id[1]);
  TEST_ASSERT_TRUE(rfkwh::parseId("29:261197", id)); // ramses notation of the same meter
  TEST_ASSERT_EQUAL_HEX8(0x77, id[0]);
  TEST_ASSERT_EQUAL_HEX8(0xFC, id[1]);
  TEST_ASSERT_EQUAL_HEX8(0x4D, id[2]);
  TEST_ASSERT_TRUE(rfkwh::parseId("29:261032", id));
  TEST_ASSERT_EQUAL_HEX8(0xA8, id[2]);
  TEST_ASSERT_FALSE(rfkwh::parseId("77,FC", id));
  TEST_ASSERT_FALSE(rfkwh::parseId("77,FC,4D,00", id));
  TEST_ASSERT_FALSE(rfkwh::parseId("zz,FC,4D", id));
  TEST_ASSERT_FALSE(rfkwh::parseId("", id));
  TEST_ASSERT_FALSE(rfkwh::parseId(nullptr, id));
}

void test_id_helpers()
{
  TEST_ASSERT_TRUE(isMeterId(ID_WP[0]));
  TEST_ASSERT_TRUE(isMeterId(ID_WTW[0]));
  TEST_ASSERT_FALSE(isMeterId(ID_WPU[0]));
  char s[12];
  formatId(ID_WP, s, sizeof(s));
  TEST_ASSERT_EQUAL_STRING("77,FC,4D", s);
  formatTopicId(ID_WTW, s, sizeof(s));
  TEST_ASSERT_EQUAL_STRING("77_FB_A8", s);
  const uint8_t low[3] = {0xA4, 0x0A, 0x01}; // leading zeros kept
  formatId(low, s, sizeof(s));
  TEST_ASSERT_EQUAL_STRING("A4,0A,01", s);
}

void test_store_update_and_new()
{
  Store st;
  Reading r;
  bool isNew = false;
  TEST_ASSERT_EQUAL(0, st.count());

  decode2401(RP_WP, 13, r);
  TEST_ASSERT_EQUAL(0, st.update(ID_WP, r, 1000, isNew));
  TEST_ASSERT_TRUE(isNew);
  TEST_ASSERT_EQUAL(1, st.count());
  TEST_ASSERT_EQUAL_UINT32(1, st.at(0).frames);

  // the repeat 1.1 s later: same slot, not new
  TEST_ASSERT_EQUAL(0, st.update(ID_WP, r, 1001, isNew));
  TEST_ASSERT_FALSE(isNew);
  TEST_ASSERT_EQUAL_UINT32(2, st.at(0).frames);
  TEST_ASSERT_EQUAL_UINT32(1001, st.at(0).lastSeen);

  decode2401(RP_WTW, 13, r);
  TEST_ASSERT_EQUAL(1, st.update(ID_WTW, r, 1002, isNew));
  TEST_ASSERT_TRUE(isNew);
  TEST_ASSERT_EQUAL(2, st.count());
  TEST_ASSERT_EQUAL_UINT32(2854836, st.at(1).reading.wh[0]);
  TEST_ASSERT_EQUAL(1, st.find(ID_WTW));
  TEST_ASSERT_EQUAL(-1, st.find(ID_WPU));

  // a new reading replaces the old one
  Reading r2 = r;
  r2.wh[0] = 2855497;
  st.update(ID_WTW, r2, 1003, isNew);
  TEST_ASSERT_EQUAL_UINT32(2855497, st.at(1).reading.wh[0]);
}

void test_store_full()
{
  Store st;
  Reading r{};
  bool isNew = false;
  uint8_t id[3] = {0xA4, 0x00, 0x00};
  for (int i = 0; i < MAX_METERS; i++)
  {
    id[2] = static_cast<uint8_t>(i);
    TEST_ASSERT_EQUAL(i, st.update(id, r, 1, isNew));
    TEST_ASSERT_TRUE(isNew);
  }
  id[2] = 0xFF;
  TEST_ASSERT_EQUAL(-1, st.update(id, r, 1, isNew));
  TEST_ASSERT_FALSE(isNew);
  TEST_ASSERT_EQUAL(MAX_METERS, st.count());
  // known meters still update when the table is full
  id[2] = 3;
  TEST_ASSERT_EQUAL(3, st.update(id, r, 2, isNew));
  st.clear();
  TEST_ASSERT_EQUAL(0, st.count());
}

int main(int, char **)
{
  UNITY_BEGIN();
  RUN_TEST(test_decode_three_counters);
  RUN_TEST(test_decode_single_counter);
  RUN_TEST(test_decode_rejects_bad_input);
  RUN_TEST(test_parse_id);
  RUN_TEST(test_id_helpers);
  RUN_TEST(test_store_update_and_new);
  RUN_TEST(test_store_full);
  return UNITY_END();
}
