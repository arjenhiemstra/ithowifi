// Phase 2 native unit tests: pure helper functions
// Tests parseHexString, compareVersions, and round()

#include <unity.h>
#include <cmath>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

// ---------------------------------------------------------------------------
// Reimplemented pure logic from the project (no hardware dependencies)
// ---------------------------------------------------------------------------

// From generic_functions.cpp — parseHexString
std::vector<int> parseHexString(const std::string &input) {
    std::vector<int> result;
    if (input.empty()) return result;
    std::stringstream ss(input);
    std::string item;
    while (std::getline(ss, item, ',')) {
        result.push_back(std::stoi(item, nullptr, 16));
    }
    return result;
}

// Helper used by parseVersion
static bool isNumeric(const std::string &s) {
    if (s.empty()) return false;
    for (char c : s) {
        if (!std::isdigit(static_cast<unsigned char>(c))) return false;
    }
    return true;
}

// From generic_functions.cpp — parseVersion
bool parseVersion(const std::string &version, std::vector<int> &numbers, std::string &prerelease) {
    size_t pos = 0, found;
    size_t hyphen = version.find('-');
    std::string numericPart = (hyphen != std::string::npos) ? version.substr(0, hyphen) : version;
    prerelease = (hyphen != std::string::npos) ? version.substr(hyphen + 1) : "";

    while ((found = numericPart.find('.', pos)) != std::string::npos) {
        std::string segment = numericPart.substr(pos, found - pos);
        if (!segment.empty() && isNumeric(segment)) {
            numbers.push_back(stoi(segment));
        } else {
            return false;
        }
        pos = found + 1;
    }

    if (pos < numericPart.length()) {
        std::string lastSegment = numericPart.substr(pos);
        if (!lastSegment.empty() && isNumeric(lastSegment)) {
            numbers.push_back(stoi(lastSegment));
        } else {
            return false;
        }
    }

    return true;
}

// From generic_functions.cpp — compareVersions
int compareVersions(const std::string &v1, const std::string &v2) {
    std::vector<int> nums1, nums2;
    std::string pre1, pre2;

    if (!parseVersion(v1, nums1, pre1) || !parseVersion(v2, nums2, pre2)) {
        return -99;
    }

    for (size_t i = 0; i < std::max(nums1.size(), nums2.size()); ++i) {
        int num1 = i < nums1.size() ? nums1[i] : 0;
        int num2 = i < nums2.size() ? nums2[i] : 0;

        if (num1 > num2) return 1;
        if (num1 < num2) return -1;
    }

    if (pre1.empty() && !pre2.empty()) return 1;   // release > pre-release
    if (!pre1.empty() && pre2.empty()) return -1;
    if (pre1.empty() && pre2.empty()) return 0;

    return pre1.compare(pre2);
}

// From generic_functions.h — round with precision
double roundPrecision(float value, int precision) {
    double pow10 = pow(10.0, precision);
    return std::round(value * pow10) / pow10;
}

// ---------------------------------------------------------------------------
// parseHexString tests
// ---------------------------------------------------------------------------

void test_parseHex_valid_3_bytes(void) {
    auto result = parseHexString("A1,34,7F");
    TEST_ASSERT_EQUAL_INT(3, (int)result.size());
    TEST_ASSERT_EQUAL_INT(0xA1, result[0]);
    TEST_ASSERT_EQUAL_INT(0x34, result[1]);
    TEST_ASSERT_EQUAL_INT(0x7F, result[2]);
}

void test_parseHex_single_byte(void) {
    auto result = parseHexString("FF");
    TEST_ASSERT_EQUAL_INT(1, (int)result.size());
    TEST_ASSERT_EQUAL_INT(0xFF, result[0]);
}

void test_parseHex_empty_string(void) {
    auto result = parseHexString("");
    TEST_ASSERT_EQUAL_INT(0, (int)result.size());
}

void test_parseHex_lowercase(void) {
    auto result = parseHexString("ab,cd,ef");
    TEST_ASSERT_EQUAL_INT(3, (int)result.size());
    TEST_ASSERT_EQUAL_INT(0xAB, result[0]);
    TEST_ASSERT_EQUAL_INT(0xCD, result[1]);
    TEST_ASSERT_EQUAL_INT(0xEF, result[2]);
}

void test_parseHex_uppercase(void) {
    auto result = parseHexString("AB,CD,EF");
    TEST_ASSERT_EQUAL_INT(3, (int)result.size());
    TEST_ASSERT_EQUAL_INT(0xAB, result[0]);
    TEST_ASSERT_EQUAL_INT(0xCD, result[1]);
    TEST_ASSERT_EQUAL_INT(0xEF, result[2]);
}

void test_parseHex_mixed_case(void) {
    auto result = parseHexString("aB,Cd,eF");
    TEST_ASSERT_EQUAL_INT(3, (int)result.size());
    TEST_ASSERT_EQUAL_INT(0xAB, result[0]);
    TEST_ASSERT_EQUAL_INT(0xCD, result[1]);
    TEST_ASSERT_EQUAL_INT(0xEF, result[2]);
}

// ---------------------------------------------------------------------------
// compareVersions tests
// ---------------------------------------------------------------------------

void test_compare_equal_versions(void) {
    TEST_ASSERT_EQUAL_INT(0, compareVersions("1.2.3", "1.2.3"));
}

void test_compare_major_higher(void) {
    TEST_ASSERT_EQUAL_INT(1, compareVersions("2.0.0", "1.9.9"));
}

void test_compare_major_lower(void) {
    TEST_ASSERT_EQUAL_INT(-1, compareVersions("1.0.0", "2.0.0"));
}

void test_compare_minor_higher(void) {
    TEST_ASSERT_EQUAL_INT(1, compareVersions("1.3.0", "1.2.9"));
}

void test_compare_minor_lower(void) {
    TEST_ASSERT_EQUAL_INT(-1, compareVersions("1.2.0", "1.3.0"));
}

void test_compare_patch_higher(void) {
    TEST_ASSERT_EQUAL_INT(1, compareVersions("1.2.4", "1.2.3"));
}

void test_compare_patch_lower(void) {
    TEST_ASSERT_EQUAL_INT(-1, compareVersions("1.2.3", "1.2.4"));
}

void test_compare_release_beats_beta(void) {
    // Release (no pre-release) is higher than beta
    TEST_ASSERT_EQUAL_INT(1, compareVersions("1.2.3", "1.2.3-beta1"));
}

void test_compare_beta_lower_than_release(void) {
    TEST_ASSERT_EQUAL_INT(-1, compareVersions("1.2.3-beta1", "1.2.3"));
}

void test_compare_beta_ordering(void) {
    // "beta1" < "beta13" lexicographically (string compare)
    TEST_ASSERT_TRUE(compareVersions("1.2.3-beta1", "1.2.3-beta13") < 0);
}

void test_compare_beta_equal(void) {
    TEST_ASSERT_EQUAL_INT(0, compareVersions("1.2.3-beta1", "1.2.3-beta1"));
}

// ---------------------------------------------------------------------------
// roundPrecision tests
// ---------------------------------------------------------------------------

void test_round_positive_2_decimals(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.14f, (float)roundPrecision(3.14159f, 2));
}

void test_round_negative_2_decimals(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.001f, -3.14f, (float)roundPrecision(-3.14159f, 2));
}

void test_round_zero_precision(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.0f, (float)roundPrecision(3.14159f, 0));
}

void test_round_1_precision(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.1f, (float)roundPrecision(3.14159f, 1));
}

void test_round_3_precision(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 3.142f, (float)roundPrecision(3.14159f, 3));
}

void test_round_4_precision(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.00001f, 3.1416f, (float)roundPrecision(3.14159f, 4));
}

void test_round_half_up(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, (float)roundPrecision(1.5f, 0));
}

void test_round_zero_value(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, (float)roundPrecision(0.0f, 2));
}

void test_round_large_value(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 12345.68f, (float)roundPrecision(12345.6789f, 2));
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main() {
    UNITY_BEGIN();

    // parseHexString
    RUN_TEST(test_parseHex_valid_3_bytes);
    RUN_TEST(test_parseHex_single_byte);
    RUN_TEST(test_parseHex_empty_string);
    RUN_TEST(test_parseHex_lowercase);
    RUN_TEST(test_parseHex_uppercase);
    RUN_TEST(test_parseHex_mixed_case);

    // compareVersions
    RUN_TEST(test_compare_equal_versions);
    RUN_TEST(test_compare_major_higher);
    RUN_TEST(test_compare_major_lower);
    RUN_TEST(test_compare_minor_higher);
    RUN_TEST(test_compare_minor_lower);
    RUN_TEST(test_compare_patch_higher);
    RUN_TEST(test_compare_patch_lower);
    RUN_TEST(test_compare_release_beats_beta);
    RUN_TEST(test_compare_beta_lower_than_release);
    RUN_TEST(test_compare_beta_ordering);
    RUN_TEST(test_compare_beta_equal);

    // roundPrecision
    RUN_TEST(test_round_positive_2_decimals);
    RUN_TEST(test_round_negative_2_decimals);
    RUN_TEST(test_round_zero_precision);
    RUN_TEST(test_round_1_precision);
    RUN_TEST(test_round_3_precision);
    RUN_TEST(test_round_4_precision);
    RUN_TEST(test_round_half_up);
    RUN_TEST(test_round_zero_value);
    RUN_TEST(test_round_large_value);

    return UNITY_END();
}
