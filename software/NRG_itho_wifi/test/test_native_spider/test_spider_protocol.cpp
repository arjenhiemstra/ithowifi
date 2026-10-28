// Native unit tests for the Itho Spider thermostat payload helpers (main/cc1101/spider_protocol.h).
// Vectors are real captures from a CCU-12T20 Spider (54:D3:FA), 2026-10-15, plus ramses_rf issue #101.

#include <unity.h>
#include <cstdint>
#include <cstring>

#include "../../main/cc1101/spider_protocol.h"

void setUp() {}
void tearDown() {}

// RQ 01FF from the Spider to its controller (two clean frames, byte 13 differs)
static const uint8_t rq01ff_a[26] = {0x03, 0x80, 0x2B, 0x28, 0x2C, 0xD0, 0x00, 0x80, 0x00, 0x14, 0x3C, 0x28, 0x40,
                                     0x00, 0x00, 0x01, 0x04, 0x80, 0x80, 0x02, 0x80, 0xFF, 0x80, 0x07, 0x00, 0x00};
static const uint8_t rq01ff_b[26] = {0x03, 0x80, 0x2B, 0x28, 0x2C, 0xD0, 0x00, 0x80, 0x00, 0x14, 0x3C, 0x28, 0x40,
                                     0x10, 0x00, 0x01, 0x04, 0x80, 0x80, 0x02, 0x80, 0xFF, 0x80, 0x07, 0x00, 0x00};
// ramses_rf #101 capture: 21.0 / 21.0 / 22.0, flags D0
static const uint8_t issue101[26] = {0x00, 0x80, 0x2A, 0x2A, 0x2C, 0xD0, 0x00, 0x80, 0x00, 0x24, 0x30, 0x80, 0x80,
                                     0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x02, 0x80, 0xFF, 0x80, 0x00, 0x00, 0xA4};

void test_decode_capture()
{
  spider::Status s{};
  TEST_ASSERT_TRUE(spider::decodeStatus(rq01ff_a, 26, s));
  TEST_ASSERT_EQUAL_INT32(2150, spider::halfToCenti(s.roomTemp));
  TEST_ASSERT_EQUAL_INT32(2000, spider::halfToCenti(s.heatSetpoint));
  TEST_ASSERT_EQUAL_INT32(2200, spider::halfToCenti(s.coolSetpoint));
  TEST_ASSERT_EQUAL_HEX8(0xD0, s.flags);
  TEST_ASSERT_TRUE(s.flags & spider::FLAG_SCHEDULE);
  TEST_ASSERT_FALSE(s.flags & spider::FLAG_MANUAL);
}

void test_decode_ignores_trailer_differences()
{
  spider::Status a{}, b{}, c{};
  TEST_ASSERT_TRUE(spider::decodeStatus(rq01ff_a, 26, a));
  TEST_ASSERT_TRUE(spider::decodeStatus(rq01ff_b, 26, b));
  TEST_ASSERT_EQUAL_MEMORY(&a, &b, sizeof(a));
  TEST_ASSERT_TRUE(spider::decodeStatus(issue101, 26, c));
  TEST_ASSERT_EQUAL_INT32(2100, spider::halfToCenti(c.roomTemp));
  TEST_ASSERT_EQUAL_INT32(2100, spider::halfToCenti(c.heatSetpoint));
  TEST_ASSERT_EQUAL_INT32(2200, spider::halfToCenti(c.coolSetpoint));
}

void test_decode_rejects_bad_length_and_placeholder()
{
  spider::Status s{};
  TEST_ASSERT_FALSE(spider::decodeStatus(rq01ff_a, 25, s));
  TEST_ASSERT_FALSE(spider::decodeStatus(rq01ff_a, 30, s)); // corrupted capture had len 0x1E
  TEST_ASSERT_FALSE(spider::decodeStatus(nullptr, 26, s));
  uint8_t ph[26];
  memcpy(ph, rq01ff_a, 26);
  ph[3] = spider::TEMP_NONE;
  TEST_ASSERT_FALSE(spider::decodeStatus(ph, 26, s));
}

void test_build_controller_status_matches_blueprint()
{
  // blueprint: 008080{sp}{sp+1}D0000000143C80800000B40080800280FF80040000, sp = 20.5 -> 29, 21.5 -> 2B
  static const uint8_t expect[26] = {0x00, 0x80, 0x80, 0x29, 0x2B, 0xD0, 0x00, 0x00, 0x00, 0x14, 0x3C, 0x80, 0x80,
                                     0x00, 0x00, 0xB4, 0x00, 0x80, 0x80, 0x02, 0x80, 0xFF, 0x80, 0x04, 0x00, 0x00};
  uint8_t out[26];
  spider::buildControllerStatus(out, 2050);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(expect, out, 26);
}

void test_controller_status_round_trip_and_clamp()
{
  uint8_t out[26];
  spider::Status s{};
  spider::buildControllerStatus(out, 1830); // rounds to 18.5
  TEST_ASSERT_TRUE(spider::decodeStatus(out, 26, s));
  TEST_ASSERT_EQUAL_INT32(1850, spider::halfToCenti(s.heatSetpoint));
  TEST_ASSERT_EQUAL_HEX8(spider::TEMP_NONE, s.roomTemp);
  spider::buildControllerStatus(out, 100);
  TEST_ASSERT_EQUAL_HEX8(10, out[3]); // 5.0 °C
  spider::buildControllerStatus(out, 4000);
  TEST_ASSERT_EQUAL_HEX8(60, out[3]); // 30.0 °C
}

void test_zone_setpoint_capture_and_echo()
{
  // I 22C9 broadcast from the Spider: 00 07D0 0898 00 -> 20.00 / 22.00
  static const uint8_t cap[6] = {0x00, 0x07, 0xD0, 0x08, 0x98, 0x00};
  int32_t c = 0;
  TEST_ASSERT_TRUE(spider::decodeZoneSetpoint(cap, 6, c));
  TEST_ASSERT_EQUAL_INT32(2000, c);
  uint8_t echo[6];
  spider::buildZoneSetpointEcho(echo, 2150);
  static const uint8_t expect[6] = {0x00, 0x08, 0x66, 0x08, 0x02, 0x01};
  TEST_ASSERT_EQUAL_HEX8_ARRAY(expect, echo, 6);
  TEST_ASSERT_TRUE(spider::decodeZoneSetpoint(echo, 6, c));
  TEST_ASSERT_EQUAL_INT32(2150, c);
}

void test_encodings_do_not_cross()
{
  // 21.5 °C is 0x2B in 01FF (half degrees) but 0x0866 in 22C9 (centidegrees)
  uint8_t st[26], zs[6];
  spider::buildControllerStatus(st, 2150);
  spider::buildZoneSetpointEcho(zs, 2150);
  TEST_ASSERT_EQUAL_HEX8(0x2B, st[3]);
  TEST_ASSERT_EQUAL_HEX8(0x08, zs[1]);
  TEST_ASSERT_EQUAL_HEX8(0x66, zs[2]);
  TEST_ASSERT_EQUAL_INT32(2150, spider::halfToCenti(st[3]));
}

void test_heating_join_detection()
{
  // this Spider's "heating" join (repeated 3x): 0008 / 3150 / 10E0 / 1FC9 -> relay variant
  static const uint8_t relay[24] = {0x00, 0x00, 0x08, 0x54, 0xD3, 0xFA, 0x00, 0x31, 0x50, 0x54, 0xD3, 0xFA,
                                    0x01, 0x10, 0xE0, 0x54, 0xD3, 0xFA, 0x00, 0x1F, 0xC9, 0x54, 0xD3, 0xFA};
  // blueprint Spider 21:033394 (ramses_cc #151): 22C9 / 22F1 / 000E / 10E0 / 1FC9 -> controller variant
  static const uint8_t ctl[30] = {0x00, 0x22, 0xC9, 0x54, 0x82, 0x72, 0x00, 0x22, 0xF1, 0x54, 0x82, 0x72,
                                  0x00, 0x00, 0x0E, 0x54, 0x82, 0x72, 0x01, 0x10, 0xE0, 0x54, 0x82, 0x72,
                                  0x00, 0x1F, 0xC9, 0x54, 0x82, 0x72};
  // ventilation join: 22F1 / 10E0 / 1FC9
  static const uint8_t vent[18] = {0x00, 0x22, 0xF1, 0x54, 0xD3, 0xFA, 0x00, 0x10, 0xE0,
                                   0x54, 0xD3, 0xFA, 0x00, 0x1F, 0xC9, 0x54, 0xD3, 0xFA};
  TEST_ASSERT_TRUE(spider::heatingJoinKind(ctl, 30) == spider::JoinKind::Controller);
  TEST_ASSERT_TRUE(spider::heatingJoinKind(relay, 24) == spider::JoinKind::Relay);
  TEST_ASSERT_TRUE(spider::heatingJoinKind(vent, 18) == spider::JoinKind::None);
  TEST_ASSERT_TRUE(spider::heatingJoinKind(relay, 23) == spider::JoinKind::None);
  TEST_ASSERT_TRUE(spider::heatingJoinKind(nullptr, 24) == spider::JoinKind::None);
}

void test_bind_accept()
{
  static const uint8_t id[3] = {0x79, 0x0C, 0x17};
  uint8_t out[6];
  spider::buildBindAccept(out, id);
  static const uint8_t expect[6] = {0x00, 0x01, 0xFF, 0x79, 0x0C, 0x17};
  TEST_ASSERT_EQUAL_HEX8_ARRAY(expect, out, 6);
}

void test_zone_setpoint_change()
{
  // W 22C9 from a user log: 22.0 C in zone 06 -> 06 0898 08FC 01
  uint8_t out[spider::ZONE_SETPOINT_ECHO_LEN];
  spider::buildZoneSetpointChange(out, 0x06, 2200);
  const uint8_t expect[6] = {0x06, 0x08, 0x98, 0x08, 0xFC, 0x01};
  TEST_ASSERT_EQUAL_HEX8_ARRAY(expect, out, 6);
  // 20.5 C -> 0802 0866; clamped to 30.0 above the range
  spider::buildZoneSetpointChange(out, 0x03, 2050);
  TEST_ASSERT_EQUAL_HEX8(0x03, out[0]);
  TEST_ASSERT_EQUAL_HEX8(0x08, out[1]);
  TEST_ASSERT_EQUAL_HEX8(0x02, out[2]);
  TEST_ASSERT_EQUAL_HEX8(0x66, out[4]);
  spider::buildZoneSetpointChange(out, 0x06, 4000);
  TEST_ASSERT_EQUAL(3000, out[1] << 8 | out[2]);
}

int main(int, char **)
{
  UNITY_BEGIN();
  RUN_TEST(test_decode_capture);
  RUN_TEST(test_decode_ignores_trailer_differences);
  RUN_TEST(test_decode_rejects_bad_length_and_placeholder);
  RUN_TEST(test_build_controller_status_matches_blueprint);
  RUN_TEST(test_controller_status_round_trip_and_clamp);
  RUN_TEST(test_zone_setpoint_capture_and_echo);
  RUN_TEST(test_encodings_do_not_cross);
  RUN_TEST(test_heating_join_detection);
  RUN_TEST(test_bind_accept);
  RUN_TEST(test_zone_setpoint_change);
  return UNITY_END();
}
