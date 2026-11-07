// Native unit tests for the pre-RAMSES Itho RF frame builder (main/cc1101/legacy_protocol.h).
// Vectors are real captures from an RFT remote joined to a CVE ECO 2 (2026-11-07), and Klusjesman's IthoEcoFanRFT tables.

#include <unity.h>
#include <cstdint>
#include <cstring>

#include "../../main/cc1101/legacy_protocol.h"

void setUp() {}
void tearDown() {}

static const uint16_t REMOTE_ID = 0x271F; // captured remote, sends 51,83,50,212,173,85 after the sync

static void assertFrame(uint16_t id, uint16_t cmd, uint8_t prev, const uint8_t (&expected)[legacy::FRAME_LEN])
{
  uint8_t frame[legacy::FRAME_LEN];
  legacy::buildFrame(id, cmd, prev, frame);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, frame, legacy::FRAME_LEN);
}

void test_captured_commands()
{
  const uint8_t low[] = {170, 170, 170, 173, 51, 83, 50, 212, 173, 85, 84, 213, 85, 83, 42, 212, 170, 171, 85};
  const uint8_t medium[] = {170, 170, 170, 173, 51, 83, 50, 212, 173, 85, 84, 213, 85, 74, 172, 180, 170, 171, 85};
  const uint8_t high[] = {170, 170, 170, 173, 51, 83, 50, 212, 173, 85, 84, 213, 85, 50, 178, 180, 170, 171, 85};
  const uint8_t timer1[] = {170, 170, 170, 173, 51, 83, 50, 212, 173, 85, 83, 83, 84, 204, 179, 52, 170, 171, 85};
  const uint8_t timer2[] = {170, 170, 170, 173, 51, 83, 50, 212, 173, 85, 83, 83, 83, 53, 77, 52, 170, 171, 85};
  const uint8_t timer3[] = {170, 170, 170, 173, 51, 83, 50, 212, 173, 85, 83, 83, 82, 173, 43, 52, 170, 171, 85};
  assertFrame(REMOTE_ID, legacy::CMD_LOW, legacy::PREV_COMMAND, low);
  assertFrame(REMOTE_ID, legacy::CMD_MEDIUM, legacy::PREV_COMMAND, medium);
  assertFrame(REMOTE_ID, legacy::CMD_HIGH, legacy::PREV_COMMAND, high);
  assertFrame(REMOTE_ID, legacy::CMD_TIMER1, legacy::PREV_COMMAND, timer1);
  assertFrame(REMOTE_ID, legacy::CMD_TIMER2, legacy::PREV_COMMAND, timer2);
  assertFrame(REMOTE_ID, legacy::CMD_TIMER3, legacy::PREV_COMMAND, timer3);
}

void test_captured_join_leave_and_previous_byte()
{
  // first repeat carries the type of the frame before, the later repeats their own type
  const uint8_t join1[] = {170, 170, 170, 173, 51, 83, 50, 212, 173, 84, 170, 171, 85, 84, 179, 52, 170, 171, 85};
  const uint8_t join2[] = {170, 170, 170, 173, 51, 83, 50, 212, 173, 84, 170, 171, 85, 84, 179, 52, 170, 171, 77};
  const uint8_t leave2[] = {170, 170, 170, 173, 51, 83, 50, 212, 173, 84, 170, 173, 85, 83, 82, 212, 170, 171, 82};
  const uint8_t lowAfterJoin[] = {170, 170, 170, 173, 51, 83, 50, 212, 173, 85, 84, 213, 85, 83, 42, 212, 170, 171, 77};
  assertFrame(REMOTE_ID, legacy::CMD_JOIN, legacy::PREV_COMMAND, join1);
  assertFrame(REMOTE_ID, legacy::CMD_JOIN, legacy::prevCode(legacy::CMD_JOIN), join2);
  assertFrame(REMOTE_ID, legacy::CMD_LEAVE, legacy::prevCode(legacy::CMD_LEAVE), leave2);
  assertFrame(REMOTE_ID, legacy::CMD_LOW, legacy::PREV_JOIN, lowAfterJoin);
}

void test_klusjesman_remote()
{
  // IthoEcoFanRFT: device id 51,83,51,43,84,204 with the message1 command tables
  const uint8_t high[] = {170, 170, 170, 173, 51, 83, 51, 43, 84, 205, 84, 213, 85, 50, 203, 52, 170, 171, 85};
  const uint8_t timer3[] = {170, 170, 170, 173, 51, 83, 51, 43, 84, 205, 83, 83, 82, 173, 82, 180, 170, 171, 85};
  const uint8_t leave[] = {170, 170, 170, 173, 51, 83, 51, 43, 84, 204, 170, 173, 85, 83, 43, 84, 170, 171, 85};
  assertFrame(0x28F5, legacy::CMD_HIGH, legacy::PREV_COMMAND, high);
  assertFrame(0x28F5, legacy::CMD_TIMER3, legacy::PREV_COMMAND, timer3);
  assertFrame(0x28F5, legacy::CMD_LEAVE, legacy::PREV_COMMAND, leave);
}

void test_crc_low_bit_is_zero()
{
  // the footer's first chip doubles as the CRC's last chip, which only works if that bit is 0
  for (uint32_t id = 0; id < 0x8000; id += 97)
    TEST_ASSERT_EQUAL_UINT8(0, legacy::crc((0x5DULL << 32) | (static_cast<uint64_t>(id) << 17) | (1ULL << 16) | legacy::CMD_LOW) & 1);
}

void test_id_is_masked_to_15_bits()
{
  uint8_t a[legacy::FRAME_LEN], b[legacy::FRAME_LEN];
  legacy::buildFrame(0xA71F, legacy::CMD_LOW, legacy::PREV_COMMAND, a);
  legacy::buildFrame(0x271F, legacy::CMD_LOW, legacy::PREV_COMMAND, b);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(b, a, legacy::FRAME_LEN);
}

int main(int, char **)
{
  UNITY_BEGIN();
  RUN_TEST(test_captured_commands);
  RUN_TEST(test_captured_join_leave_and_previous_byte);
  RUN_TEST(test_klusjesman_remote);
  RUN_TEST(test_crc_low_bit_is_zero);
  RUN_TEST(test_id_is_masked_to_15_bits);
  return UNITY_END();
}
