/**
 * Native unit tests for speed/timer parsing and command matching logic.
 *
 * Reimplements the pure parsing logic from generic_functions.cpp
 * (ithoSetSpeed, ithoSetTimer, ithoSetSpeedTimer, ithoExecCommand)
 * without hardware dependencies.
 *
 * Build: pio test -e native_test --filter test_native_speed_parse
 */
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <unity.h>

// ---------------------------------------------------------------------------
// Reimplemented parsing logic extracted from generic_functions.cpp
// ---------------------------------------------------------------------------

// ithoSetSpeed(const char*) does: uint16_t val = strtoul(speed, NULL, 10);
// ithoSetSpeed(uint16_t) checks: if (speed < 256) { accept } else { reject }
bool parseSpeed(const char *str, uint16_t &result) {
    if (str == nullptr || *str == '\0') return false;
    char *end;
    unsigned long val = strtoul(str, &end, 10);
    if (end == str) return false;   // no digits parsed
    if (*end != '\0') return false; // trailing characters
    if (val >= 256) return false;   // mirrors: if (speed < 256)
    result = static_cast<uint16_t>(val);
    return true;
}

// ithoSetTimer(const char*) does: uint16_t t = strtoul(timer, NULL, 10);
// ithoSetTimer(uint16_t) checks: if (timer > 0 && timer < 65535)
bool parseTimer(const char *str, uint16_t &result) {
    if (str == nullptr || *str == '\0') return false;
    char *end;
    unsigned long val = strtoul(str, &end, 10);
    if (end == str) return false;
    if (*end != '\0') return false;
    if (val == 0 || val >= 65535) return false; // mirrors: timer > 0 && timer < 65535
    result = static_cast<uint16_t>(val);
    return true;
}

// ithoSetSpeedTimer(const char*, const char*) converts both, then:
// ithoSetSpeedTimer(uint16_t, uint16_t) checks: if (speed < 255)
// Note: speed limit is 255 here, not 256 as in standalone setSpeed
bool parseSpeedTimer(const char *speed_str, const char *timer_str,
                     uint16_t &speed, uint16_t &timer) {
    if (speed_str == nullptr || *speed_str == '\0') return false;
    if (timer_str == nullptr || *timer_str == '\0') return false;
    char *end1, *end2;
    unsigned long sv = strtoul(speed_str, &end1, 10);
    unsigned long tv = strtoul(timer_str, &end2, 10);
    if (end1 == speed_str || *end1 != '\0') return false;
    if (end2 == timer_str || *end2 != '\0') return false;
    if (sv >= 255) return false; // mirrors: if (speed < 255)
    speed = static_cast<uint16_t>(sv);
    timer = static_cast<uint16_t>(tv);
    return true;
}

// ithoExecCommand non-i2c branch: command string matching
// Returns true if the command is recognized
bool execCommand(const char *command) {
    if (command == nullptr) return false;
    if (strcmp(command, "away") == 0) return true;
    if (strcmp(command, "low") == 0) return true;
    if (strcmp(command, "medium") == 0) return true;
    if (strcmp(command, "high") == 0) return true;
    if (strcmp(command, "timer1") == 0) return true;
    if (strcmp(command, "timer2") == 0) return true;
    if (strcmp(command, "timer3") == 0) return true;
    if (strcmp(command, "cook30") == 0) return true;
    if (strcmp(command, "cook60") == 0) return true;
    if (strcmp(command, "auto") == 0) return true;
    if (strcmp(command, "autonight") == 0) return true;
    if (strcmp(command, "clearqueue") == 0) return true;
    return false;
}

// apiCmdAllowed from MqttAPI.cpp -- superset that includes RF commands
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
// Speed parsing tests
// ---------------------------------------------------------------------------

void test_speed_valid_zero(void) {
    uint16_t v;
    TEST_ASSERT_TRUE(parseSpeed("0", v));
    TEST_ASSERT_EQUAL(0, v);
}

void test_speed_valid_1(void) {
    uint16_t v;
    TEST_ASSERT_TRUE(parseSpeed("1", v));
    TEST_ASSERT_EQUAL(1, v);
}

void test_speed_valid_120(void) {
    uint16_t v;
    TEST_ASSERT_TRUE(parseSpeed("120", v));
    TEST_ASSERT_EQUAL(120, v);
}

void test_speed_valid_254(void) {
    uint16_t v;
    TEST_ASSERT_TRUE(parseSpeed("254", v));
    TEST_ASSERT_EQUAL(254, v);
}

void test_speed_valid_255(void) {
    uint16_t v;
    // Real code: speed < 256, so 255 is accepted
    TEST_ASSERT_TRUE(parseSpeed("255", v));
    TEST_ASSERT_EQUAL(255, v);
}

void test_speed_invalid_256(void) {
    uint16_t v;
    TEST_ASSERT_FALSE(parseSpeed("256", v));
}

void test_speed_invalid_999(void) {
    uint16_t v;
    TEST_ASSERT_FALSE(parseSpeed("999", v));
}

void test_speed_invalid_empty(void) {
    uint16_t v;
    TEST_ASSERT_FALSE(parseSpeed("", v));
}

void test_speed_invalid_null(void) {
    uint16_t v;
    TEST_ASSERT_FALSE(parseSpeed(nullptr, v));
}

void test_speed_invalid_alpha(void) {
    uint16_t v;
    TEST_ASSERT_FALSE(parseSpeed("abc", v));
}

void test_speed_invalid_negative(void) {
    uint16_t v;
    // strtoul("-1") wraps to large value >= 256
    TEST_ASSERT_FALSE(parseSpeed("-1", v));
}

void test_speed_invalid_float(void) {
    uint16_t v;
    TEST_ASSERT_FALSE(parseSpeed("12.5", v));
}

void test_speed_leading_space(void) {
    uint16_t v;
    // strtoul skips leading whitespace, so " 100" parses as 100
    TEST_ASSERT_TRUE(parseSpeed(" 100", v));
    TEST_ASSERT_EQUAL(100, v);
}

void test_speed_invalid_trailing_text(void) {
    uint16_t v;
    TEST_ASSERT_FALSE(parseSpeed("100abc", v));
}

void test_speed_valid_boundary_low(void) {
    uint16_t v;
    TEST_ASSERT_TRUE(parseSpeed("0", v));
    TEST_ASSERT_EQUAL(0, v);
}

// ---------------------------------------------------------------------------
// Timer parsing tests
// ---------------------------------------------------------------------------

void test_timer_valid_1(void) {
    uint16_t v;
    TEST_ASSERT_TRUE(parseTimer("1", v));
    TEST_ASSERT_EQUAL(1, v);
}

void test_timer_valid_300(void) {
    uint16_t v;
    TEST_ASSERT_TRUE(parseTimer("300", v));
    TEST_ASSERT_EQUAL(300, v);
}

void test_timer_valid_65534(void) {
    uint16_t v;
    // Real code: timer < 65535, so 65534 is the max accepted
    TEST_ASSERT_TRUE(parseTimer("65534", v));
    TEST_ASSERT_EQUAL(65534, v);
}

void test_timer_invalid_zero(void) {
    uint16_t v;
    // Real code: timer > 0, so 0 is rejected
    TEST_ASSERT_FALSE(parseTimer("0", v));
}

void test_timer_invalid_65535(void) {
    uint16_t v;
    // Real code: timer < 65535, so 65535 is rejected
    TEST_ASSERT_FALSE(parseTimer("65535", v));
}

void test_timer_invalid_65536(void) {
    uint16_t v;
    TEST_ASSERT_FALSE(parseTimer("65536", v));
}

void test_timer_invalid_alpha(void) {
    uint16_t v;
    TEST_ASSERT_FALSE(parseTimer("abc", v));
}

void test_timer_invalid_negative(void) {
    uint16_t v;
    TEST_ASSERT_FALSE(parseTimer("-1", v));
}

void test_timer_invalid_null(void) {
    uint16_t v;
    TEST_ASSERT_FALSE(parseTimer(nullptr, v));
}

void test_timer_invalid_empty(void) {
    uint16_t v;
    TEST_ASSERT_FALSE(parseTimer("", v));
}

void test_timer_invalid_float(void) {
    uint16_t v;
    TEST_ASSERT_FALSE(parseTimer("10.5", v));
}

// ---------------------------------------------------------------------------
// Combined speed+timer parsing tests
// ---------------------------------------------------------------------------

void test_speedtimer_both_valid(void) {
    uint16_t s, t;
    TEST_ASSERT_TRUE(parseSpeedTimer("100", "300", s, t));
    TEST_ASSERT_EQUAL(100, s);
    TEST_ASSERT_EQUAL(300, t);
}

void test_speedtimer_speed_at_limit(void) {
    uint16_t s, t;
    // Combined uses speed < 255, so 254 is max
    TEST_ASSERT_TRUE(parseSpeedTimer("254", "60", s, t));
    TEST_ASSERT_EQUAL(254, s);
    TEST_ASSERT_EQUAL(60, t);
}

void test_speedtimer_speed_255_rejected(void) {
    uint16_t s, t;
    // Combined function: speed < 255, so 255 is rejected
    TEST_ASSERT_FALSE(parseSpeedTimer("255", "300", s, t));
}

void test_speedtimer_bad_speed(void) {
    uint16_t s, t;
    TEST_ASSERT_FALSE(parseSpeedTimer("999", "30", s, t));
}

void test_speedtimer_bad_timer(void) {
    uint16_t s, t;
    TEST_ASSERT_FALSE(parseSpeedTimer("100", "abc", s, t));
}

void test_speedtimer_both_bad(void) {
    uint16_t s, t;
    TEST_ASSERT_FALSE(parseSpeedTimer("abc", "xyz", s, t));
}

void test_speedtimer_null_speed(void) {
    uint16_t s, t;
    TEST_ASSERT_FALSE(parseSpeedTimer(nullptr, "10", s, t));
}

void test_speedtimer_null_timer(void) {
    uint16_t s, t;
    TEST_ASSERT_FALSE(parseSpeedTimer("10", nullptr, s, t));
}

void test_speedtimer_zero_speed(void) {
    uint16_t s, t;
    TEST_ASSERT_TRUE(parseSpeedTimer("0", "10", s, t));
    TEST_ASSERT_EQUAL(0, s);
    TEST_ASSERT_EQUAL(10, t);
}

// ---------------------------------------------------------------------------
// Command matching tests (ithoExecCommand non-i2c branch)
// ---------------------------------------------------------------------------

void test_cmd_low(void) { TEST_ASSERT_TRUE(execCommand("low")); }
void test_cmd_medium(void) { TEST_ASSERT_TRUE(execCommand("medium")); }
void test_cmd_high(void) { TEST_ASSERT_TRUE(execCommand("high")); }
void test_cmd_away(void) { TEST_ASSERT_TRUE(execCommand("away")); }
void test_cmd_timer1(void) { TEST_ASSERT_TRUE(execCommand("timer1")); }
void test_cmd_timer2(void) { TEST_ASSERT_TRUE(execCommand("timer2")); }
void test_cmd_timer3(void) { TEST_ASSERT_TRUE(execCommand("timer3")); }
void test_cmd_cook30(void) { TEST_ASSERT_TRUE(execCommand("cook30")); }
void test_cmd_cook60(void) { TEST_ASSERT_TRUE(execCommand("cook60")); }
void test_cmd_auto(void) { TEST_ASSERT_TRUE(execCommand("auto")); }
void test_cmd_autonight(void) { TEST_ASSERT_TRUE(execCommand("autonight")); }
void test_cmd_clearqueue(void) { TEST_ASSERT_TRUE(execCommand("clearqueue")); }
void test_cmd_unknown(void) { TEST_ASSERT_FALSE(execCommand("unknown_cmd")); }
void test_cmd_empty(void) { TEST_ASSERT_FALSE(execCommand("")); }
void test_cmd_null(void) { TEST_ASSERT_FALSE(execCommand(nullptr)); }
void test_cmd_case_sensitive_Low(void) { TEST_ASSERT_FALSE(execCommand("Low")); }
void test_cmd_case_sensitive_HIGH(void) { TEST_ASSERT_FALSE(execCommand("HIGH")); }
void test_cmd_case_sensitive_Medium(void) { TEST_ASSERT_FALSE(execCommand("Medium")); }
void test_cmd_partial(void) { TEST_ASSERT_FALSE(execCommand("tim")); }

// apiCmdAllowed includes RF-only commands that execCommand does not
void test_api_motion_on(void) { TEST_ASSERT_TRUE(apiCmdAllowed("motion_on")); }
void test_api_motion_off(void) { TEST_ASSERT_TRUE(apiCmdAllowed("motion_off")); }
void test_api_join(void) { TEST_ASSERT_TRUE(apiCmdAllowed("join")); }
void test_api_leave(void) { TEST_ASSERT_TRUE(apiCmdAllowed("leave")); }

// These commands are NOT in execCommand (non-i2c) but ARE in apiCmdAllowed
void test_exec_rejects_motion_on(void) { TEST_ASSERT_FALSE(execCommand("motion_on")); }
void test_exec_rejects_join(void) { TEST_ASSERT_FALSE(execCommand("join")); }
void test_exec_rejects_leave(void) { TEST_ASSERT_FALSE(execCommand("leave")); }

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    UNITY_BEGIN();

    // Speed parsing (15 tests)
    RUN_TEST(test_speed_valid_zero);
    RUN_TEST(test_speed_valid_1);
    RUN_TEST(test_speed_valid_120);
    RUN_TEST(test_speed_valid_254);
    RUN_TEST(test_speed_valid_255);
    RUN_TEST(test_speed_valid_boundary_low);
    RUN_TEST(test_speed_invalid_256);
    RUN_TEST(test_speed_invalid_999);
    RUN_TEST(test_speed_invalid_empty);
    RUN_TEST(test_speed_invalid_null);
    RUN_TEST(test_speed_invalid_alpha);
    RUN_TEST(test_speed_invalid_negative);
    RUN_TEST(test_speed_invalid_float);
    RUN_TEST(test_speed_leading_space);
    RUN_TEST(test_speed_invalid_trailing_text);

    // Timer parsing (11 tests)
    RUN_TEST(test_timer_valid_1);
    RUN_TEST(test_timer_valid_300);
    RUN_TEST(test_timer_valid_65534);
    RUN_TEST(test_timer_invalid_zero);
    RUN_TEST(test_timer_invalid_65535);
    RUN_TEST(test_timer_invalid_65536);
    RUN_TEST(test_timer_invalid_alpha);
    RUN_TEST(test_timer_invalid_negative);
    RUN_TEST(test_timer_invalid_null);
    RUN_TEST(test_timer_invalid_empty);
    RUN_TEST(test_timer_invalid_float);

    // Combined speed+timer (9 tests)
    RUN_TEST(test_speedtimer_both_valid);
    RUN_TEST(test_speedtimer_speed_at_limit);
    RUN_TEST(test_speedtimer_speed_255_rejected);
    RUN_TEST(test_speedtimer_bad_speed);
    RUN_TEST(test_speedtimer_bad_timer);
    RUN_TEST(test_speedtimer_both_bad);
    RUN_TEST(test_speedtimer_null_speed);
    RUN_TEST(test_speedtimer_null_timer);
    RUN_TEST(test_speedtimer_zero_speed);

    // Command matching (22 tests)
    RUN_TEST(test_cmd_low);
    RUN_TEST(test_cmd_medium);
    RUN_TEST(test_cmd_high);
    RUN_TEST(test_cmd_away);
    RUN_TEST(test_cmd_timer1);
    RUN_TEST(test_cmd_timer2);
    RUN_TEST(test_cmd_timer3);
    RUN_TEST(test_cmd_cook30);
    RUN_TEST(test_cmd_cook60);
    RUN_TEST(test_cmd_auto);
    RUN_TEST(test_cmd_autonight);
    RUN_TEST(test_cmd_clearqueue);
    RUN_TEST(test_cmd_unknown);
    RUN_TEST(test_cmd_empty);
    RUN_TEST(test_cmd_null);
    RUN_TEST(test_cmd_case_sensitive_Low);
    RUN_TEST(test_cmd_case_sensitive_HIGH);
    RUN_TEST(test_cmd_case_sensitive_Medium);
    RUN_TEST(test_cmd_partial);
    RUN_TEST(test_api_motion_on);
    RUN_TEST(test_api_motion_off);
    RUN_TEST(test_api_join);
    RUN_TEST(test_api_leave);
    RUN_TEST(test_exec_rejects_motion_on);
    RUN_TEST(test_exec_rejects_join);
    RUN_TEST(test_exec_rejects_leave);

    return UNITY_END();
}
