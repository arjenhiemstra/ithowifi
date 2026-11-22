// Native unit tests for the servicetool bridge wire protocol
// (main/managers/ServicetoolProtocol.h), using example frames in the format
// of the Itho interface cable.

#include <unity.h>
#include <vector>
#include <cstring>

#include "../../main/managers/ServicetoolProtocol.h"

using namespace servicetool;

// Example 0x4030 write frame that contains a 0x10 data byte:
// 82 80 C0 30 06 0B 00 00 10 29 00 0A 00 00 01 01 00 B8
static const uint8_t kFrame4030[] = {0x82, 0x80, 0xC0, 0x30, 0x06, 0x0B, 0x00, 0x00, 0x10, 0x29,
                                     0x00, 0x0A, 0x00, 0x00, 0x01, 0x01, 0x00, 0xB8};
// Same frame on the serial line (note the doubled 0x10)
static const uint8_t kWire4030[] = {0x10, 0x02, 0x82, 0x80, 0xC0, 0x30, 0x06, 0x0B, 0x00, 0x00, 0x10, 0x10,
                                    0x29, 0x00, 0x0A, 0x00, 0x00, 0x01, 0x01, 0x00, 0xB8, 0x10, 0x03};
// Device type query used by ithowifi
static const uint8_t kQuery90E0[] = {0x82, 0x80, 0x90, 0xE0, 0x04, 0x00, 0x8A};

void setUp() {}
void tearDown() {}

void test_checksum_matches_reference()
{
  TEST_ASSERT_EQUAL_HEX8(0xB8, frameChecksum(kFrame4030, sizeof(kFrame4030) - 1));
  TEST_ASSERT_EQUAL_HEX8(0x8A, frameChecksum(kQuery90E0, sizeof(kQuery90E0) - 1));
}

void test_frame_valid()
{
  TEST_ASSERT_TRUE(frameValid(kFrame4030, sizeof(kFrame4030)));
  TEST_ASSERT_TRUE(frameValid(kQuery90E0, sizeof(kQuery90E0)));

  uint8_t bad[sizeof(kFrame4030)];
  memcpy(bad, kFrame4030, sizeof(bad));
  bad[sizeof(bad) - 1] ^= 0x01; // wrong checksum
  TEST_ASSERT_FALSE(frameValid(bad, sizeof(bad)));
  TEST_ASSERT_FALSE(frameValid(kFrame4030, sizeof(kFrame4030) - 1)); // length mismatch
  TEST_ASSERT_FALSE(frameValid(kFrame4030, 3));
}

void test_class_strips_high_bit()
{
  TEST_ASSERT_EQUAL_HEX16(0x4030, frameClass(kFrame4030));
  TEST_ASSERT_EQUAL_HEX16(0x10E0, frameClass(kQuery90E0));
}

void test_blocked_classes()
{
  TEST_ASSERT_TRUE(classBlocked(0x4035));
  TEST_ASSERT_TRUE(classBlocked(0x4038));
  TEST_ASSERT_TRUE(classBlocked(0x4F35));
  TEST_ASSERT_TRUE(classBlocked(0x4F37));
  TEST_ASSERT_FALSE(classBlocked(0x4030));
  TEST_ASSERT_FALSE(classBlocked(0x2410));
  TEST_ASSERT_FALSE(classBlocked(0x4F3E));
}

void test_encode_matches_wire_format()
{
  uint8_t out[64];
  size_t n = dleEncode(kFrame4030, sizeof(kFrame4030), out, sizeof(out));
  TEST_ASSERT_EQUAL_UINT32(sizeof(kWire4030), n);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(kWire4030, out, n);

  TEST_ASSERT_EQUAL_UINT32(0, dleEncode(kFrame4030, sizeof(kFrame4030), out, 10)); // too small
}

void test_decoder_frame_and_ping()
{
  DleDecoder d;
  // leading noise and a ping
  const uint8_t pre[] = {0xAA, 0x10, 0x16};
  TEST_ASSERT_TRUE(d.feed(pre[0]) == DleDecoder::Event::None);
  TEST_ASSERT_TRUE(d.feed(pre[1]) == DleDecoder::Event::None);
  TEST_ASSERT_TRUE(d.feed(pre[2]) == DleDecoder::Event::Ping);

  DleDecoder::Event last = DleDecoder::Event::None;
  int frames = 0;
  for (uint8_t b : kWire4030)
  {
    last = d.feed(b);
    if (last == DleDecoder::Event::Frame)
      frames++;
  }
  TEST_ASSERT_EQUAL_INT(1, frames);
  TEST_ASSERT_TRUE(last == DleDecoder::Event::Frame);
  TEST_ASSERT_EQUAL_UINT32(sizeof(kFrame4030), d.size());
  TEST_ASSERT_EQUAL_HEX8_ARRAY(kFrame4030, d.data(), d.size());
}

void test_decoder_roundtrip_all_bytes()
{
  // frame containing every byte value incl. many 0x10
  std::vector<uint8_t> f = {0x82, 0x80, 0x24, 0x10, 0x04, 0x00};
  std::vector<uint8_t> data;
  for (int i = 0; i < 200; i++)
    data.push_back(static_cast<uint8_t>(i % 3 == 0 ? 0x10 : i));
  f[5] = static_cast<uint8_t>(data.size());
  f.insert(f.end(), data.begin(), data.end());
  f.push_back(frameChecksum(f.data(), f.size()));
  TEST_ASSERT_TRUE(frameValid(f.data(), f.size()));

  std::vector<uint8_t> wire(f.size() * 2 + 4);
  size_t n = dleEncode(f.data(), f.size(), wire.data(), wire.size());
  TEST_ASSERT_TRUE(n > 0);

  DleDecoder d;
  bool got = false;
  for (size_t i = 0; i < n; i++)
    if (d.feed(wire[i]) == DleDecoder::Event::Frame)
      got = true;
  TEST_ASSERT_TRUE(got);
  TEST_ASSERT_EQUAL_UINT32(f.size(), d.size());
  TEST_ASSERT_EQUAL_HEX8_ARRAY(f.data(), d.data(), f.size());
}

void test_decoder_overflow()
{
  DleDecoder d;
  d.feed(DLE);
  d.feed(STX);
  for (size_t i = 0; i < MAX_FRAME + 10; i++)
    d.feed(0x01);
  d.feed(DLE);
  TEST_ASSERT_TRUE(d.feed(ETX) == DleDecoder::Event::Overflow);
}

void test_split_single_and_double_reply()
{
  // reply as collected by i2cSlaveReceive: our address 0x80 prepended
  std::vector<uint8_t> r1 = {0x80, 0x82, 0x90, 0xE0, 0x01, 0x02, 0x00, 0x0D};
  r1.push_back(frameChecksum(r1.data(), r1.size()));

  std::vector<std::vector<uint8_t>> got;
  auto cb = [&](const uint8_t *p, size_t len)
  { got.emplace_back(p, p + len); };

  TEST_ASSERT_EQUAL_UINT32(1, splitReceived(r1.data(), r1.size(), 0x80, cb));
  TEST_ASSERT_EQUAL_UINT32(1, got.size());
  TEST_ASSERT_TRUE(frameValid(got[0].data(), got[0].size()));

  // second frame in the same window arrives without its destination byte
  std::vector<uint8_t> f2 = {0x80, 0x82, 0x24, 0x01, 0x01, 0x01, 0x07};
  f2.push_back(frameChecksum(f2.data(), f2.size()));
  std::vector<uint8_t> both = r1;
  both.insert(both.end(), f2.begin() + 1, f2.end());

  got.clear();
  TEST_ASSERT_EQUAL_UINT32(2, splitReceived(both.data(), both.size(), 0x80, cb));
  TEST_ASSERT_EQUAL_UINT32(2, got.size());
  TEST_ASSERT_EQUAL_HEX8_ARRAY(f2.data(), got[1].data(), f2.size());
  TEST_ASSERT_TRUE(frameValid(got[1].data(), got[1].size()));

  got.clear();
  TEST_ASSERT_EQUAL_UINT32(0, splitReceived(r1.data(), 4, 0x80, cb)); // truncated
}

void test_reply_window()
{
  TEST_ASSERT_EQUAL_UINT32(2000, replyWindowMs(kQuery90E0)); // type 4 query
  TEST_ASSERT_EQUAL_UINT32(1000, replyWindowMs(kFrame4030)); // type 6 change
  const uint8_t report[] = {0x00, 0x3E, 0xCE, 0x30, 0x05, 0x00, 0x00};
  TEST_ASSERT_EQUAL_UINT32(50, replyWindowMs(report));       // type 5, no reply
  // node identification query to the general call address 0x00 (seen at
  // servicetool start-up) - must wait for the Itho's answer
  const uint8_t bcastQuery[] = {0x00, 0x80, 0x90, 0xE0, 0x04, 0x00, 0x0C};
  TEST_ASSERT_EQUAL_UINT32(2000, replyWindowMs(bcastQuery));
}

int main(int, char **)
{
  UNITY_BEGIN();
  RUN_TEST(test_checksum_matches_reference);
  RUN_TEST(test_frame_valid);
  RUN_TEST(test_class_strips_high_bit);
  RUN_TEST(test_blocked_classes);
  RUN_TEST(test_encode_matches_wire_format);
  RUN_TEST(test_decoder_frame_and_ping);
  RUN_TEST(test_decoder_roundtrip_all_bytes);
  RUN_TEST(test_decoder_overflow);
  RUN_TEST(test_split_single_and_double_reply);
  RUN_TEST(test_reply_window);
  return UNITY_END();
}
