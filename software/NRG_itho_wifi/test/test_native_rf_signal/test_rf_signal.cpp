// Native unit tests for the RF signal diagnostics helpers (main/cc1101/RFSignal.h):
// CC1101 RSSI / FREQEST / LQI conversions per the datasheet and the RFSTAT receive counters.

#include <unity.h>
#include <cstdint>

#include "../../main/cc1101/RFSignal.h"

void setUp() {}
void tearDown() {}

void test_rssi_positive_register_values()
{
  TEST_ASSERT_EQUAL_INT(-74, rfsignal::rssiDbm(0));    // 0 -> offset only
  TEST_ASSERT_EQUAL_INT(-64, rfsignal::rssiDbm(20));   // +10 dB
  TEST_ASSERT_EQUAL_INT(-11, rfsignal::rssiDbm(127));  // strongest
}

void test_rssi_negative_register_values()
{
  TEST_ASSERT_EQUAL_INT(-75, rfsignal::rssiDbm(0xFF)); // -0.5 dB rounds down
  TEST_ASSERT_EQUAL_INT(-106, rfsignal::rssiDbm(0xC0)); // -64 -> -32 dB
  TEST_ASSERT_EQUAL_INT(-138, rfsignal::rssiDbm(0x80)); // weakest
}

void test_rssi_is_monotonic()
{
  // stronger register value (as signed) never gives a weaker dBm
  int prev = rfsignal::rssiDbm(0x80);
  for (int v = -127; v <= 127; v++)
  {
    const int now = rfsignal::rssiDbm(static_cast<uint8_t>(v));
    TEST_ASSERT_TRUE(now >= prev);
    prev = now;
  }
}

void test_freq_offset_two_complement()
{
  TEST_ASSERT_EQUAL_INT32(0, rfsignal::freqOffsetHz(0));
  TEST_ASSERT_EQUAL_INT32(1586, rfsignal::freqOffsetHz(1));   // 26 MHz / 2^14 = 1586.9 Hz
  TEST_ASSERT_EQUAL_INT32(-1586, rfsignal::freqOffsetHz(0xFF));
  TEST_ASSERT_EQUAL_INT32(-203125, rfsignal::freqOffsetHz(0x80));
}

void test_link_quality_masks_crc_bit()
{
  TEST_ASSERT_EQUAL_UINT8(5, rfsignal::linkQuality(0x85));
  TEST_ASSERT_EQUAL_UINT8(0x7F, rfsignal::linkQuality(0xFF));
}

void test_counters()
{
  rfsignal::Counters c;
  rfsignal::count(c, 0, true);   // good, known
  rfsignal::count(c, 0, false);  // good, foreign
  rfsignal::count(c, 1, false);  // header error (noise)
  rfsignal::count(c, 2, false);  // checksum error, foreign
  rfsignal::count(c, 2, true);   // checksum error from a registered device
  TEST_ASSERT_EQUAL_UINT32(5, c.decoded);
  TEST_ASSERT_EQUAL_UINT32(1, c.hdrErr);
  TEST_ASSERT_EQUAL_UINT32(2, c.chkErr);
  TEST_ASSERT_EQUAL_UINT32(2, c.known);
  TEST_ASSERT_EQUAL_UINT32(1, c.knownErr);
  TEST_ASSERT_EQUAL_UINT32(0, c.sync);
  TEST_ASSERT_EQUAL_UINT32(0, c.coalesced);
}

int main(int argc, char **argv)
{
  UNITY_BEGIN();
  RUN_TEST(test_rssi_positive_register_values);
  RUN_TEST(test_rssi_negative_register_values);
  RUN_TEST(test_rssi_is_monotonic);
  RUN_TEST(test_freq_offset_two_complement);
  RUN_TEST(test_link_quality_masks_crc_bit);
  RUN_TEST(test_counters);
  return UNITY_END();
}
