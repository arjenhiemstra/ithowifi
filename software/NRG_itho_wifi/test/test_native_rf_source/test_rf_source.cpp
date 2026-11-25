// Native tests for ithoStatusSourceId: which address a 31D9/31DA status frame is filed under (#406)

#include <unity.h>
#include <cstdint>

#include "../../main/cc1101/IthoPacket.h"

void setUp(void) {}
void tearDown(void) {}

void test_addr0_and_addr2_equal(void)
{
  TEST_ASSERT_EQUAL_HEX32(0x4926F3, ithoStatusSourceId(0x4926F3, 0x4926F3));
}

void test_addr0_only(void)
{
  TEST_ASSERT_EQUAL_HEX32(0x4926F3, ithoStatusSourceId(0x4926F3, 0));
}

void test_addr2_only(void)
{
  TEST_ASSERT_EQUAL_HEX32(0x4926F3, ithoStatusSourceId(0, 0x4926F3));
}

void test_no_address(void)
{
  TEST_ASSERT_EQUAL_HEX32(0, ithoStatusSourceId(0, 0));
}

void test_bit_error_in_addr0_is_dropped(void)
{
  // neighbour 49:24:F3 with a bit error in addr0, as captured in #406
  TEST_ASSERT_EQUAL_HEX32(0, ithoStatusSourceId(0x4964F3, 0x4924F3));
  TEST_ASSERT_EQUAL_HEX32(0, ithoStatusSourceId(0x4926F3, 0x4924F3));
}

int main(int argc, char **argv)
{
  UNITY_BEGIN();
  RUN_TEST(test_addr0_and_addr2_equal);
  RUN_TEST(test_addr0_only);
  RUN_TEST(test_addr2_only);
  RUN_TEST(test_no_address);
  RUN_TEST(test_bit_error_in_addr0_is_dropped);
  return UNITY_END();
}
