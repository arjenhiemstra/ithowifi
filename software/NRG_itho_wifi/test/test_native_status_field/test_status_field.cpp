// Native unit tests: one 2401 status field across polls
// Runs the firmware's own ithoStatusUpdateField (main/ithodevice/IthoStatusField.h).

#include <unity.h>
#include <cmath>

#include "../../main/ithodevice/IthoStatusField.h"

static const char *NOT_AVAILABLE = "not available";

static ithoDeviceStatus makeField(const char *name, decltype(ithoDeviceStatus::type) type,
                                  uint8_t length, uint32_t divider, bool is_signed)
{
    ithoDeviceStatus f;
    f.name = name;
    f.type = type;
    f.length = length;
    f.divider = divider;
    f.is_signed = is_signed;
    f.value.intval = 0;
    return f;
}

void setUp() {}
void tearDown() {}

// ---------------------------------------------------------------------------
// Decoding
// ---------------------------------------------------------------------------

void test_float_field_applies_divider()
{
    auto f = makeField("Fan speed (rpm)", ithoDeviceStatus::is_float, 2, 100, false);
    ithoStatusUpdateField(f, 4714, NOT_AVAILABLE);
    TEST_ASSERT_EQUAL(ithoDeviceStatus::is_float, f.type);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 47.14f, f.value.floatval);
}

void test_signed_int_field_reads_negative()
{
    auto f = makeField("Delta", ithoDeviceStatus::is_int, 2, 1, true);
    ithoStatusUpdateField(f, 0xFFFE, NOT_AVAILABLE);
    TEST_ASSERT_EQUAL_INT32(-2, f.value.intval);
}

void test_signed_float_field_reads_negative()
{
    auto f = makeField("Outdoor temp", ithoDeviceStatus::is_float, 2, 100, true);
    ithoStatusUpdateField(f, 0xFF38, NOT_AVAILABLE);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -2.0f, f.value.floatval);
}

void test_byte_field_stores_low_byte()
{
    auto f = makeField("Error", ithoDeviceStatus::is_byte, 1, 1, false);
    ithoStatusUpdateField(f, 0x107, NOT_AVAILABLE);
    TEST_ASSERT_EQUAL_UINT8(7, f.value.byteval);
}

int main()
{
    UNITY_BEGIN();

    RUN_TEST(test_float_field_applies_divider);
    RUN_TEST(test_signed_int_field_reads_negative);
    RUN_TEST(test_signed_float_field_reads_negative);
    RUN_TEST(test_byte_field_stores_low_byte);

    return UNITY_END();
}
