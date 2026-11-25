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

// ---------------------------------------------------------------------------
// Sensor errors are judged per poll (issue #295)
// ---------------------------------------------------------------------------

void test_humidity_recovers_after_one_out_of_range_read()
{
    auto f = makeField("RelativeHumidity", ithoDeviceStatus::is_float, 2, 100, false);
    ithoStatusUpdateField(f, 4714, NOT_AVAILABLE);
    ithoStatusUpdateField(f, 0, NOT_AVAILABLE);
    ithoStatusUpdateField(f, 4690, NOT_AVAILABLE);
    TEST_ASSERT_EQUAL(ithoDeviceStatus::is_float, f.type);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 46.90f, f.value.floatval);
}

void test_temperature_recovers_after_one_out_of_range_read()
{
    auto f = makeField("Temperature", ithoDeviceStatus::is_float, 2, 100, true);
    ithoStatusUpdateField(f, 2249, NOT_AVAILABLE);
    ithoStatusUpdateField(f, 0, NOT_AVAILABLE);
    ithoStatusUpdateField(f, 2251, NOT_AVAILABLE);
    TEST_ASSERT_EQUAL(ithoDeviceStatus::is_float, f.type);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 22.51f, f.value.floatval);
}

void test_out_of_range_humidity_shows_sensor_error()
{
    auto f = makeField("RelativeHumidity", ithoDeviceStatus::is_float, 2, 100, false);
    ithoStatusUpdateField(f, 0, NOT_AVAILABLE);
    TEST_ASSERT_EQUAL_STRING(NOT_AVAILABLE, f.error);
}

void test_out_of_range_temperature_shows_sensor_error()
{
    auto f = makeField("Temperature", ithoDeviceStatus::is_float, 2, 100, true);
    ithoStatusUpdateField(f, 10001, NOT_AVAILABLE);
    TEST_ASSERT_EQUAL_STRING(NOT_AVAILABLE, f.error);
}

void test_valid_read_clears_sensor_error()
{
    auto f = makeField("RelativeHumidity", ithoDeviceStatus::is_float, 2, 100, false);
    ithoStatusUpdateField(f, 0, NOT_AVAILABLE);
    ithoStatusUpdateField(f, 4690, NOT_AVAILABLE);
    TEST_ASSERT_NULL(f.error);
}

void test_co2_sensor_error_code_shows_error_then_recovers()
{
    auto f = makeField("Highest CO2 concentration (ppm)", ithoDeviceStatus::is_int, 2, 1, false);
    ithoStatusUpdateField(f, 0x8200, NOT_AVAILABLE);
    TEST_ASSERT_EQUAL_STRING(NOT_AVAILABLE, f.error);
    ithoStatusUpdateField(f, 420, NOT_AVAILABLE);
    TEST_ASSERT_NULL(f.error);
    TEST_ASSERT_EQUAL(ithoDeviceStatus::is_int, f.type);
    TEST_ASSERT_EQUAL_INT32(420, f.value.intval);
}

void test_highest_rh_sensor_error_code_shows_error_then_recovers()
{
    auto f = makeField("Highest RH concentration (%)", ithoDeviceStatus::is_int, 1, 1, false);
    ithoStatusUpdateField(f, 0xEF, NOT_AVAILABLE);
    TEST_ASSERT_EQUAL_STRING(NOT_AVAILABLE, f.error);
    ithoStatusUpdateField(f, 46, NOT_AVAILABLE);
    TEST_ASSERT_NULL(f.error);
    TEST_ASSERT_EQUAL(ithoDeviceStatus::is_int, f.type);
    TEST_ASSERT_EQUAL_INT32(46, f.value.intval);
}

void test_normalized_labels_get_the_same_sensor_checks()
{
    auto rh = makeField("relativehumidity", ithoDeviceStatus::is_float, 2, 100, false);
    ithoStatusUpdateField(rh, 0, NOT_AVAILABLE);
    TEST_ASSERT_EQUAL_STRING(NOT_AVAILABLE, rh.error);

    auto temp = makeField("temperature", ithoDeviceStatus::is_float, 2, 100, true);
    ithoStatusUpdateField(temp, 0, NOT_AVAILABLE);
    TEST_ASSERT_EQUAL_STRING(NOT_AVAILABLE, temp.error);

    auto co2 = makeField("highest-co2-concentration_ppm", ithoDeviceStatus::is_int, 2, 1, false);
    ithoStatusUpdateField(co2, 0x8200, NOT_AVAILABLE);
    TEST_ASSERT_EQUAL_STRING(NOT_AVAILABLE, co2.error);

    auto rhMax = makeField("highest-rh-concentration_perc", ithoDeviceStatus::is_int, 1, 1, false);
    ithoStatusUpdateField(rhMax, 0xEF, NOT_AVAILABLE);
    TEST_ASSERT_EQUAL_STRING(NOT_AVAILABLE, rhMax.error);
}

int main()
{
    UNITY_BEGIN();

    RUN_TEST(test_float_field_applies_divider);
    RUN_TEST(test_signed_int_field_reads_negative);
    RUN_TEST(test_signed_float_field_reads_negative);
    RUN_TEST(test_byte_field_stores_low_byte);

    RUN_TEST(test_humidity_recovers_after_one_out_of_range_read);
    RUN_TEST(test_temperature_recovers_after_one_out_of_range_read);
    RUN_TEST(test_out_of_range_humidity_shows_sensor_error);
    RUN_TEST(test_out_of_range_temperature_shows_sensor_error);
    RUN_TEST(test_valid_read_clears_sensor_error);
    RUN_TEST(test_co2_sensor_error_code_shows_error_then_recovers);
    RUN_TEST(test_highest_rh_sensor_error_code_shows_error_then_recovers);
    RUN_TEST(test_normalized_labels_get_the_same_sensor_checks);

    return UNITY_END();
}
