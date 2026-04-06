// Phase 2 native unit tests: UUID parsing and formatting
// Tests uuidUnparse (format bytes to string) and uuidParse (string to bytes)

#include <unity.h>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cctype>

// ---------------------------------------------------------------------------
// Reimplemented UUID logic
// ---------------------------------------------------------------------------

// Format 16 bytes as "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx" (36 chars + null)
void uuidUnparse(const uint8_t uuid[16], char *out) {
    snprintf(out, 37, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        uuid[0], uuid[1], uuid[2], uuid[3],
        uuid[4], uuid[5],
        uuid[6], uuid[7],
        uuid[8], uuid[9],
        uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);
}

// Parse UUID string back to 16 bytes. Returns 0 on success, -1 on error.
int uuidParse(const char *input, uint8_t uuid[16]) {
    if (!input) return -1;

    // Validate length (36 chars: 8-4-4-4-12)
    size_t len = strlen(input);
    if (len != 36) return -1;

    // Validate hyphen positions
    if (input[8] != '-' || input[13] != '-' || input[18] != '-' || input[23] != '-')
        return -1;

    // Validate all other chars are hex
    for (size_t i = 0; i < 36; i++) {
        if (i == 8 || i == 13 || i == 18 || i == 23) continue;
        if (!isxdigit((unsigned char)input[i])) return -1;
    }

    // Parse hex pairs
    const int offsets[] = {0, 2, 4, 6, 9, 11, 14, 16, 19, 21, 24, 26, 28, 30, 32, 34};
    for (int i = 0; i < 16; i++) {
        char hex[3] = {input[offsets[i]], input[offsets[i] + 1], '\0'};
        uuid[i] = (uint8_t)strtoul(hex, nullptr, 16);
    }

    return 0;
}

// ---------------------------------------------------------------------------
// uuidUnparse tests
// ---------------------------------------------------------------------------

void test_unparse_all_zeros(void) {
    uint8_t uuid[16] = {0};
    char out[37];
    uuidUnparse(uuid, out);
    TEST_ASSERT_EQUAL_STRING("00000000-0000-0000-0000-000000000000", out);
}

void test_unparse_known_bytes(void) {
    uint8_t uuid[16] = {
        0x01, 0x23, 0x45, 0x67,
        0x89, 0xab,
        0xcd, 0xef,
        0x01, 0x23,
        0x45, 0x67, 0x89, 0xab, 0xcd, 0xef
    };
    char out[37];
    uuidUnparse(uuid, out);
    TEST_ASSERT_EQUAL_STRING("01234567-89ab-cdef-0123-456789abcdef", out);
}

void test_unparse_all_ff(void) {
    uint8_t uuid[16];
    memset(uuid, 0xFF, 16);
    char out[37];
    uuidUnparse(uuid, out);
    TEST_ASSERT_EQUAL_STRING("ffffffff-ffff-ffff-ffff-ffffffffffff", out);
}

void test_unparse_mixed_values(void) {
    uint8_t uuid[16] = {
        0xDE, 0xAD, 0xBE, 0xEF,
        0xCA, 0xFE,
        0xBA, 0xBE,
        0x00, 0x11,
        0x22, 0x33, 0x44, 0x55, 0x66, 0x77
    };
    char out[37];
    uuidUnparse(uuid, out);
    TEST_ASSERT_EQUAL_STRING("deadbeef-cafe-babe-0011-223344556677", out);
}

// ---------------------------------------------------------------------------
// uuidParse tests
// ---------------------------------------------------------------------------

void test_parse_valid_uuid(void) {
    uint8_t uuid[16] = {0};
    int result = uuidParse("01234567-89ab-cdef-0123-456789abcdef", uuid);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_HEX8(0x01, uuid[0]);
    TEST_ASSERT_EQUAL_HEX8(0x23, uuid[1]);
    TEST_ASSERT_EQUAL_HEX8(0x45, uuid[2]);
    TEST_ASSERT_EQUAL_HEX8(0x67, uuid[3]);
    TEST_ASSERT_EQUAL_HEX8(0x89, uuid[4]);
    TEST_ASSERT_EQUAL_HEX8(0xAB, uuid[5]);
    TEST_ASSERT_EQUAL_HEX8(0xCD, uuid[6]);
    TEST_ASSERT_EQUAL_HEX8(0xEF, uuid[7]);
    TEST_ASSERT_EQUAL_HEX8(0x01, uuid[8]);
    TEST_ASSERT_EQUAL_HEX8(0x23, uuid[9]);
    TEST_ASSERT_EQUAL_HEX8(0x45, uuid[10]);
    TEST_ASSERT_EQUAL_HEX8(0x67, uuid[11]);
    TEST_ASSERT_EQUAL_HEX8(0x89, uuid[12]);
    TEST_ASSERT_EQUAL_HEX8(0xAB, uuid[13]);
    TEST_ASSERT_EQUAL_HEX8(0xCD, uuid[14]);
    TEST_ASSERT_EQUAL_HEX8(0xEF, uuid[15]);
}

void test_parse_uppercase_hex(void) {
    uint8_t uuid[16] = {0};
    int result = uuidParse("DEADBEEF-CAFE-BABE-0011-223344556677", uuid);
    TEST_ASSERT_EQUAL_INT(0, result);
    TEST_ASSERT_EQUAL_HEX8(0xDE, uuid[0]);
    TEST_ASSERT_EQUAL_HEX8(0xAD, uuid[1]);
    TEST_ASSERT_EQUAL_HEX8(0xBE, uuid[2]);
    TEST_ASSERT_EQUAL_HEX8(0xEF, uuid[3]);
}

void test_parse_unparse_roundtrip(void) {
    const char *original = "a1b2c3d4-e5f6-0718-293a-4b5c6d7e8f90";
    uint8_t uuid[16] = {0};
    int result = uuidParse(original, uuid);
    TEST_ASSERT_EQUAL_INT(0, result);

    char out[37];
    uuidUnparse(uuid, out);
    TEST_ASSERT_EQUAL_STRING(original, out);
}

void test_parse_all_zeros_roundtrip(void) {
    const char *original = "00000000-0000-0000-0000-000000000000";
    uint8_t uuid[16] = {0xFF}; // start with non-zero
    int result = uuidParse(original, uuid);
    TEST_ASSERT_EQUAL_INT(0, result);

    char out[37];
    uuidUnparse(uuid, out);
    TEST_ASSERT_EQUAL_STRING(original, out);
}

// ---------------------------------------------------------------------------
// uuidParse error handling tests
// ---------------------------------------------------------------------------

void test_parse_wrong_length_short(void) {
    uint8_t uuid[16];
    int result = uuidParse("01234567-89ab-cdef-0123", uuid);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_parse_wrong_length_long(void) {
    uint8_t uuid[16];
    int result = uuidParse("01234567-89ab-cdef-0123-456789abcdef0", uuid);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_parse_missing_hyphens(void) {
    uint8_t uuid[16];
    int result = uuidParse("0123456789abcdef0123456789abcdef0000", uuid);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_parse_wrong_hyphen_positions(void) {
    uint8_t uuid[16];
    // Hyphens in wrong positions (length 36 but misplaced)
    int result = uuidParse("012345678-9ab-cdef-012-3456789abcdef", uuid);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_parse_bad_hex_chars(void) {
    uint8_t uuid[16];
    int result = uuidParse("GGGGGGGG-GGGG-GGGG-GGGG-GGGGGGGGGGGG", uuid);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_parse_null_input(void) {
    uint8_t uuid[16];
    int result = uuidParse(nullptr, uuid);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

void test_parse_empty_string(void) {
    uint8_t uuid[16];
    int result = uuidParse("", uuid);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main() {
    UNITY_BEGIN();

    // uuidUnparse
    RUN_TEST(test_unparse_all_zeros);
    RUN_TEST(test_unparse_known_bytes);
    RUN_TEST(test_unparse_all_ff);
    RUN_TEST(test_unparse_mixed_values);

    // uuidParse
    RUN_TEST(test_parse_valid_uuid);
    RUN_TEST(test_parse_uppercase_hex);
    RUN_TEST(test_parse_unparse_roundtrip);
    RUN_TEST(test_parse_all_zeros_roundtrip);

    // Error handling
    RUN_TEST(test_parse_wrong_length_short);
    RUN_TEST(test_parse_wrong_length_long);
    RUN_TEST(test_parse_missing_hyphens);
    RUN_TEST(test_parse_wrong_hyphen_positions);
    RUN_TEST(test_parse_bad_hex_chars);
    RUN_TEST(test_parse_null_input);
    RUN_TEST(test_parse_empty_string);

    return UNITY_END();
}
