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

// Receive buffer as the CC1101 delivers it in normal legacy mode: everything after the 170,173 sync
static const uint8_t CAPTURED_LOW[] = {51, 83, 50, 212, 173, 85, 84, 213, 85, 83, 42, 212, 170, 171, 85, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void test_decode_captured_frame()
{
  const legacy::Decoded d = legacy::decodeFrame(CAPTURED_LOW, sizeof(CAPTURED_LOW), 0);
  TEST_ASSERT_TRUE(d.manchesterOk);
  TEST_ASSERT_EQUAL_HEX8(0x5D, d.header);
  TEST_ASSERT_EQUAL_HEX16(REMOTE_ID, d.id);
  TEST_ASSERT_EQUAL_UINT8(1, d.type);
  TEST_ASSERT_EQUAL_HEX16(legacy::CMD_LOW, d.cmd);
  TEST_ASSERT_EQUAL_HEX8(d.crcCalc, d.crc);
  TEST_ASSERT_TRUE(d.footerOk);
  TEST_ASSERT_TRUE(d.hasPrev);
  TEST_ASSERT_EQUAL_UINT8(legacy::PREV_COMMAND, d.prev);
  TEST_ASSERT_EQUAL_STRING("low", legacy::cmdName(d.cmd));
}

void test_decode_round_trip()
{
  const uint16_t cmds[] = {legacy::CMD_LOW, legacy::CMD_MEDIUM, legacy::CMD_HIGH, legacy::CMD_TIMER1,
                           legacy::CMD_TIMER2, legacy::CMD_TIMER3, legacy::CMD_JOIN, legacy::CMD_LEAVE};
  for (uint16_t id = 0; id < 0x8000; id += 1237)
    for (uint16_t cmd : cmds)
    {
      uint8_t frame[legacy::FRAME_LEN];
      legacy::buildFrame(id, cmd, legacy::PREV_JOIN, frame);
      const legacy::Decoded d = legacy::decodeFrame(frame + 4, legacy::FRAME_LEN - 4, 0);
      TEST_ASSERT_TRUE(d.manchesterOk);
      TEST_ASSERT_EQUAL_HEX16(id, d.id);
      TEST_ASSERT_EQUAL_HEX16(cmd, d.cmd);
      TEST_ASSERT_EQUAL_UINT8(legacy::isJoinLeave(cmd) ? 0 : 1, d.type);
      TEST_ASSERT_EQUAL_HEX8(d.crcCalc, d.crc);
      TEST_ASSERT_TRUE(d.footerOk);
      TEST_ASSERT_EQUAL_UINT8(legacy::PREV_JOIN, d.prev);
    }
}

void test_decode_reports_errors()
{
  uint8_t buf[sizeof(CAPTURED_LOW)];
  // one chip flipped: not a Manchester pair any more
  memcpy(buf, CAPTURED_LOW, sizeof(buf));
  buf[3] ^= 0x10; // chip 27
  legacy::Decoded d = legacy::decodeFrame(buf, sizeof(buf), 0);
  TEST_ASSERT_FALSE(d.manchesterOk);
  TEST_ASSERT_EQUAL_UINT16(27, d.badChip);
  // both chips of a pair flipped: valid Manchester, wrong bit, CRC no longer matches
  memcpy(buf, CAPTURED_LOW, sizeof(buf));
  buf[3] ^= 0x18; // chips 27 and 28 (pairs start at odd chips, after the padding chip)
  d = legacy::decodeFrame(buf, sizeof(buf), 0);
  TEST_ASSERT_TRUE(d.manchesterOk);
  TEST_ASSERT_NOT_EQUAL(d.crcCalc, d.crc);
  // too short for the footer and the previous-type byte
  d = legacy::decodeFrame(CAPTURED_LOW, 13, 0); // 104 chips: all 48 bits, not the whole footer
  TEST_ASSERT_TRUE(d.manchesterOk);
  TEST_ASSERT_FALSE(d.footerOk);
  TEST_ASSERT_FALSE(d.hasPrev);
  d = legacy::decodeFrame(CAPTURED_LOW, 12, 0); // 96 chips: one short of the 48th bit
  TEST_ASSERT_FALSE(d.manchesterOk);
  TEST_ASSERT_EQUAL_UINT16(95, d.badChip);
}

// Loose mode syncs on 170,170 somewhere in the preamble, so the buffer starts in it at an even chip
static void shiftChips(const uint8_t *in, size_t inLen, size_t fromChip, uint8_t *out, size_t outLen)
{
  memset(out, 0, outLen);
  for (size_t c = 0; c < outLen * 8 && fromChip + c < inLen * 8; c++)
    if (legacy::chipAt(in, fromChip + c))
      out[c >> 3] |= 0x80 >> (c & 7);
}

void test_find_frame_start_in_loose_capture()
{
  uint8_t frame[legacy::FRAME_LEN];
  legacy::buildFrame(REMOTE_ID, legacy::CMD_HIGH, legacy::PREV_COMMAND, frame);
  for (size_t from : {16, 18, 22, 26})
  {
    uint8_t buf[32];
    shiftChips(frame, sizeof(frame), from, buf, sizeof(buf));
    const int start = legacy::findFrameStart(buf, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(32 - from), start);
    const legacy::Decoded d = legacy::decodeFrame(buf, sizeof(buf), start);
    TEST_ASSERT_TRUE(d.manchesterOk);
    TEST_ASSERT_EQUAL_HEX16(legacy::CMD_HIGH, d.cmd);
    TEST_ASSERT_EQUAL_HEX16(REMOTE_ID, d.id);
  }
  // a preamble that does not end in 173 is not a frame start
  const uint8_t other[] = {170, 170, 171, 0x33, 0x55};
  TEST_ASSERT_EQUAL_INT(-1, legacy::findFrameStart(other, sizeof(other)));
}

int main(int, char **)
{
  UNITY_BEGIN();
  RUN_TEST(test_captured_commands);
  RUN_TEST(test_captured_join_leave_and_previous_byte);
  RUN_TEST(test_klusjesman_remote);
  RUN_TEST(test_crc_low_bit_is_zero);
  RUN_TEST(test_id_is_masked_to_15_bits);
  RUN_TEST(test_decode_captured_frame);
  RUN_TEST(test_decode_round_trip);
  RUN_TEST(test_decode_reports_errors);
  RUN_TEST(test_find_frame_start_in_loose_capture);
  return UNITY_END();
}
