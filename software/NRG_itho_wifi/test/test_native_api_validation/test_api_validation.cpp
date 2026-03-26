/**
 * Native unit tests for API parameter validation logic.
 *
 * Reimplements validation patterns extracted from WebAPIv2.cpp
 * (processGetsettingCommands, processSetsettingCommands, processSetRFremote)
 * and apiCmdAllowed from MqttAPI.cpp.
 *
 * Build: pio test -e native_test --filter test_native_api_validation
 */
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <string>
#include <vector>
#include <unity.h>
#include <ArduinoJson.h>

// ---------------------------------------------------------------------------
// Reimplemented validation logic from WebAPIv2.cpp
// ---------------------------------------------------------------------------

// Index validation (from processGetsettingCommands / processSetsettingCommands)
// Real code: std::stoul(str, &pos), checks pos == str.size(), then idx >= max
bool validateIndex(const char *value, unsigned long maxIndex, unsigned long &result) {
    if (value == nullptr || *value == '\0') return false;
    try {
        size_t pos;
        result = std::stoul(value, &pos);
        if (pos != strlen(value)) return false;  // trailing chars
        if (result >= maxIndex) return false;
        return true;
    } catch (...) { return false; }
}

// Integer range validation (from processSetsettingCommands is_int branch)
// Real code: std::stoul(new_val_str, &pos), checks pos != size, then val < min || val > max
bool validateIntRange(const char *value, int32_t minVal, int32_t maxVal, int32_t &result) {
    if (value == nullptr || *value == '\0') return false;
    try {
        size_t pos;
        long val = std::stol(value, &pos);
        if (pos != strlen(value)) return false;  // trailing chars
        if (val < minVal || val > maxVal) return false;
        result = static_cast<int32_t>(val);
        return true;
    } catch (...) { return false; }
}

// Float range validation (from processSetsettingCommands is_float branch)
// Real code: std::stod(new_val_str), then checks val < min || val > max
bool validateFloatRange(const char *value, double minVal, double maxVal, double &result) {
    if (value == nullptr || *value == '\0') return false;
    try {
        size_t pos;
        double val = std::stod(value, &pos);
        if (pos != strlen(value)) return false;
        if (val < minVal || val > maxVal) return false;
        result = val;
        return true;
    } catch (...) { return false; }
}

// Hex string parsing (from generic_functions.cpp hexStringToInt + parseHexString)
int hexStringToInt(const std::string &hexStr) {
    int result = 0;
    for (char c : hexStr) {
        result *= 16;
        if (c >= '0' && c <= '9') result += (c - '0');
        else if (c >= 'A' && c <= 'F') result += (c - 'A' + 10);
        else if (c >= 'a' && c <= 'f') result += (c - 'a' + 10);
        // invalid chars silently contribute 0 (matches real code)
    }
    return result;
}

std::vector<int> parseHexString(const std::string &input) {
    std::vector<int> result;
    if (input.empty()) {
        result.push_back(0); // real code always adds substr(start), even for empty
        return result;
    }
    size_t start = 0;
    size_t end = input.find(',');
    while (end != std::string::npos) {
        std::string hexStr = input.substr(start, end - start);
        result.push_back(hexStringToInt(hexStr));
        start = end + 1;
        end = input.find(',', start);
    }
    std::string hexStr = input.substr(start);
    result.push_back(hexStringToInt(hexStr));
    return result;
}

// Hex ID validation: used in processSetRFremote, checks id.size() == 3
bool validateHexId(const std::string &input) {
    auto parts = parseHexString(input);
    return parts.size() == 3;
}

// apiCmdAllowed from MqttAPI.cpp (verbatim logic)
bool apiCmdAllowed(const char *cmd) {
    if (cmd == nullptr) return false;
    const char *apicmds[] = {"low", "medium", "auto", "high", "timer1", "timer2", "timer3",
                              "away", "cook30", "cook60", "autonight", "motion_on", "motion_off",
                              "join", "leave", "clearqueue"};
    for (uint8_t i = 0; i < sizeof(apicmds) / sizeof(apicmds[0]); i++) {
        if (strcmp(cmd, apicmds[i]) == 0) return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Index validation tests
// ---------------------------------------------------------------------------

void test_index_valid_zero(void) {
    unsigned long r;
    TEST_ASSERT_TRUE(validateIndex("0", 256, r));
    TEST_ASSERT_EQUAL(0, r);
}

void test_index_valid_100(void) {
    unsigned long r;
    TEST_ASSERT_TRUE(validateIndex("100", 256, r));
    TEST_ASSERT_EQUAL(100, r);
}

void test_index_valid_255(void) {
    unsigned long r;
    TEST_ASSERT_TRUE(validateIndex("255", 256, r));
    TEST_ASSERT_EQUAL(255, r);
}

void test_index_at_max_boundary(void) {
    unsigned long r;
    // result >= maxIndex fails, so 256 with max=256 is rejected
    TEST_ASSERT_FALSE(validateIndex("256", 256, r));
}

void test_index_over_max(void) {
    unsigned long r;
    TEST_ASSERT_FALSE(validateIndex("999", 256, r));
}

void test_index_alpha(void) {
    unsigned long r;
    TEST_ASSERT_FALSE(validateIndex("abc", 256, r));
}

void test_index_empty(void) {
    unsigned long r;
    TEST_ASSERT_FALSE(validateIndex("", 256, r));
}

void test_index_null(void) {
    unsigned long r;
    TEST_ASSERT_FALSE(validateIndex(nullptr, 256, r));
}

void test_index_trailing_chars(void) {
    unsigned long r;
    TEST_ASSERT_FALSE(validateIndex("12abc", 256, r));
}

void test_index_negative(void) {
    unsigned long r;
    // stoul with "-1" throws or wraps depending on implementation
    TEST_ASSERT_FALSE(validateIndex("-1", 256, r));
}

void test_index_float_string(void) {
    unsigned long r;
    TEST_ASSERT_FALSE(validateIndex("1.5", 256, r));
}

void test_index_small_max(void) {
    unsigned long r;
    TEST_ASSERT_TRUE(validateIndex("0", 1, r));
    TEST_ASSERT_EQUAL(0, r);
    TEST_ASSERT_FALSE(validateIndex("1", 1, r));
}

// ---------------------------------------------------------------------------
// Integer range validation tests
// ---------------------------------------------------------------------------

void test_int_in_range(void) {
    int32_t r;
    TEST_ASSERT_TRUE(validateIntRange("50", 0, 100, r));
    TEST_ASSERT_EQUAL(50, r);
}

void test_int_at_min(void) {
    int32_t r;
    TEST_ASSERT_TRUE(validateIntRange("0", 0, 255, r));
    TEST_ASSERT_EQUAL(0, r);
}

void test_int_at_max(void) {
    int32_t r;
    TEST_ASSERT_TRUE(validateIntRange("255", 0, 255, r));
    TEST_ASSERT_EQUAL(255, r);
}

void test_int_below_min(void) {
    int32_t r;
    TEST_ASSERT_FALSE(validateIntRange("-1", 0, 255, r));
}

void test_int_above_max(void) {
    int32_t r;
    TEST_ASSERT_FALSE(validateIntRange("256", 0, 255, r));
}

void test_int_negative_range(void) {
    int32_t r;
    TEST_ASSERT_TRUE(validateIntRange("-50", -100, 100, r));
    TEST_ASSERT_EQUAL(-50, r);
}

void test_int_at_negative_min(void) {
    int32_t r;
    TEST_ASSERT_TRUE(validateIntRange("-100", -100, 100, r));
    TEST_ASSERT_EQUAL(-100, r);
}

void test_int_below_negative_min(void) {
    int32_t r;
    TEST_ASSERT_FALSE(validateIntRange("-101", -100, 100, r));
}

void test_int_alpha(void) {
    int32_t r;
    TEST_ASSERT_FALSE(validateIntRange("abc", 0, 100, r));
}

void test_int_empty(void) {
    int32_t r;
    TEST_ASSERT_FALSE(validateIntRange("", 0, 100, r));
}

void test_int_null(void) {
    int32_t r;
    TEST_ASSERT_FALSE(validateIntRange(nullptr, 0, 100, r));
}

void test_int_trailing_chars(void) {
    int32_t r;
    TEST_ASSERT_FALSE(validateIntRange("50abc", 0, 100, r));
}

// ---------------------------------------------------------------------------
// Float range validation tests
// ---------------------------------------------------------------------------

void test_float_valid(void) {
    double r;
    TEST_ASSERT_TRUE(validateFloatRange("10.5", -100.0, 100.0, r));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 10.5f, (float)r);
}

void test_float_at_min_boundary(void) {
    double r;
    TEST_ASSERT_TRUE(validateFloatRange("-100", -100.0, 100.0, r));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -100.0f, (float)r);
}

void test_float_at_max_boundary(void) {
    double r;
    TEST_ASSERT_TRUE(validateFloatRange("100", -100.0, 100.0, r));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 100.0f, (float)r);
}

void test_float_below_range(void) {
    double r;
    TEST_ASSERT_FALSE(validateFloatRange("-101", -100.0, 100.0, r));
}

void test_float_above_range(void) {
    double r;
    TEST_ASSERT_FALSE(validateFloatRange("101", -100.0, 100.0, r));
}

void test_float_alpha(void) {
    double r;
    TEST_ASSERT_FALSE(validateFloatRange("abc", -100.0, 100.0, r));
}

void test_float_empty(void) {
    double r;
    TEST_ASSERT_FALSE(validateFloatRange("", -100.0, 100.0, r));
}

void test_float_null(void) {
    double r;
    TEST_ASSERT_FALSE(validateFloatRange(nullptr, -100.0, 100.0, r));
}

void test_float_negative_value(void) {
    double r;
    TEST_ASSERT_TRUE(validateFloatRange("-25.7", -100.0, 100.0, r));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -25.7f, (float)r);
}

// ---------------------------------------------------------------------------
// Hex ID validation tests
// ---------------------------------------------------------------------------

void test_hex_valid_3byte(void) {
    TEST_ASSERT_TRUE(validateHexId("A1,34,7F"));
}

void test_hex_valid_lowercase(void) {
    TEST_ASSERT_TRUE(validateHexId("a1,34,7f"));
}

void test_hex_too_short_2byte(void) {
    TEST_ASSERT_FALSE(validateHexId("A1,34"));
}

void test_hex_too_long_4byte(void) {
    TEST_ASSERT_FALSE(validateHexId("A1,34,7F,00"));
}

void test_hex_single_byte(void) {
    TEST_ASSERT_FALSE(validateHexId("FF"));
}

void test_hex_empty(void) {
    // Empty string: parseHexString pushes one element (hexStringToInt("") = 0), size=1 != 3
    TEST_ASSERT_FALSE(validateHexId(""));
}

void test_hex_typical_device_id(void) {
    TEST_ASSERT_TRUE(validateHexId("3A,D1,FD"));
}

void test_hex_zeros(void) {
    TEST_ASSERT_TRUE(validateHexId("00,00,00"));
}

// ---------------------------------------------------------------------------
// apiCmdAllowed tests
// ---------------------------------------------------------------------------

void test_api_low(void) { TEST_ASSERT_TRUE(apiCmdAllowed("low")); }
void test_api_medium(void) { TEST_ASSERT_TRUE(apiCmdAllowed("medium")); }
void test_api_auto(void) { TEST_ASSERT_TRUE(apiCmdAllowed("auto")); }
void test_api_high(void) { TEST_ASSERT_TRUE(apiCmdAllowed("high")); }
void test_api_timer1(void) { TEST_ASSERT_TRUE(apiCmdAllowed("timer1")); }
void test_api_timer2(void) { TEST_ASSERT_TRUE(apiCmdAllowed("timer2")); }
void test_api_timer3(void) { TEST_ASSERT_TRUE(apiCmdAllowed("timer3")); }
void test_api_away(void) { TEST_ASSERT_TRUE(apiCmdAllowed("away")); }
void test_api_cook30(void) { TEST_ASSERT_TRUE(apiCmdAllowed("cook30")); }
void test_api_cook60(void) { TEST_ASSERT_TRUE(apiCmdAllowed("cook60")); }
void test_api_autonight(void) { TEST_ASSERT_TRUE(apiCmdAllowed("autonight")); }
void test_api_motion_on(void) { TEST_ASSERT_TRUE(apiCmdAllowed("motion_on")); }
void test_api_motion_off(void) { TEST_ASSERT_TRUE(apiCmdAllowed("motion_off")); }
void test_api_join(void) { TEST_ASSERT_TRUE(apiCmdAllowed("join")); }
void test_api_leave(void) { TEST_ASSERT_TRUE(apiCmdAllowed("leave")); }
void test_api_clearqueue(void) { TEST_ASSERT_TRUE(apiCmdAllowed("clearqueue")); }
void test_api_invalid_reboot(void) { TEST_ASSERT_FALSE(apiCmdAllowed("reboot")); }
void test_api_invalid_empty(void) { TEST_ASSERT_FALSE(apiCmdAllowed("")); }
void test_api_case_sensitive(void) { TEST_ASSERT_FALSE(apiCmdAllowed("LOW")); }
void test_api_null(void) { TEST_ASSERT_FALSE(apiCmdAllowed(nullptr)); }

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    UNITY_BEGIN();

    // Index validation (12 tests)
    RUN_TEST(test_index_valid_zero);
    RUN_TEST(test_index_valid_100);
    RUN_TEST(test_index_valid_255);
    RUN_TEST(test_index_at_max_boundary);
    RUN_TEST(test_index_over_max);
    RUN_TEST(test_index_alpha);
    RUN_TEST(test_index_empty);
    RUN_TEST(test_index_null);
    RUN_TEST(test_index_trailing_chars);
    RUN_TEST(test_index_negative);
    RUN_TEST(test_index_float_string);
    RUN_TEST(test_index_small_max);

    // Integer range validation (12 tests)
    RUN_TEST(test_int_in_range);
    RUN_TEST(test_int_at_min);
    RUN_TEST(test_int_at_max);
    RUN_TEST(test_int_below_min);
    RUN_TEST(test_int_above_max);
    RUN_TEST(test_int_negative_range);
    RUN_TEST(test_int_at_negative_min);
    RUN_TEST(test_int_below_negative_min);
    RUN_TEST(test_int_alpha);
    RUN_TEST(test_int_empty);
    RUN_TEST(test_int_null);
    RUN_TEST(test_int_trailing_chars);

    // Float range validation (9 tests)
    RUN_TEST(test_float_valid);
    RUN_TEST(test_float_at_min_boundary);
    RUN_TEST(test_float_at_max_boundary);
    RUN_TEST(test_float_below_range);
    RUN_TEST(test_float_above_range);
    RUN_TEST(test_float_alpha);
    RUN_TEST(test_float_empty);
    RUN_TEST(test_float_null);
    RUN_TEST(test_float_negative_value);

    // Hex ID validation (8 tests)
    RUN_TEST(test_hex_valid_3byte);
    RUN_TEST(test_hex_valid_lowercase);
    RUN_TEST(test_hex_too_short_2byte);
    RUN_TEST(test_hex_too_long_4byte);
    RUN_TEST(test_hex_single_byte);
    RUN_TEST(test_hex_empty);
    RUN_TEST(test_hex_typical_device_id);
    RUN_TEST(test_hex_zeros);

    // apiCmdAllowed (20 tests)
    RUN_TEST(test_api_low);
    RUN_TEST(test_api_medium);
    RUN_TEST(test_api_auto);
    RUN_TEST(test_api_high);
    RUN_TEST(test_api_timer1);
    RUN_TEST(test_api_timer2);
    RUN_TEST(test_api_timer3);
    RUN_TEST(test_api_away);
    RUN_TEST(test_api_cook30);
    RUN_TEST(test_api_cook60);
    RUN_TEST(test_api_autonight);
    RUN_TEST(test_api_motion_on);
    RUN_TEST(test_api_motion_off);
    RUN_TEST(test_api_join);
    RUN_TEST(test_api_leave);
    RUN_TEST(test_api_clearqueue);
    RUN_TEST(test_api_invalid_reboot);
    RUN_TEST(test_api_invalid_empty);
    RUN_TEST(test_api_case_sensitive);
    RUN_TEST(test_api_null);

    return UNITY_END();
}
