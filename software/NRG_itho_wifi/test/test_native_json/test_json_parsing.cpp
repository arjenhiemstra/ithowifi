/**
 * Native unit tests for JSON parsing logic.
 *
 * Tests JSend format construction, command validation, and parameter
 * range checking without any ESP32 hardware dependencies.
 *
 * Build: pio test -e native_test --filter test_native/test_json_parsing
 */
#include <unity.h>
#include <ArduinoJson.h>

// ---------------------------------------------------------------------------
// JSend format tests
// ---------------------------------------------------------------------------

void test_jsend_success_format(void) {
    JsonDocument doc;
    doc["status"] = "success";
    JsonObject data = doc["data"].to<JsonObject>();
    data["currentspeed"] = 42;
    data["timestamp"] = 1700000000;

    TEST_ASSERT_EQUAL_STRING("success", doc["status"].as<const char *>());
    TEST_ASSERT_TRUE(doc["data"].is<JsonObject>());
    TEST_ASSERT_EQUAL(42, doc["data"]["currentspeed"].as<int>());
    TEST_ASSERT_EQUAL(1700000000, doc["data"]["timestamp"].as<long>());
}

void test_jsend_fail_format(void) {
    JsonDocument doc;
    doc["status"] = "fail";
    JsonObject data = doc["data"].to<JsonObject>();
    data["failreason"] = "speed out of range (0-255)";
    data["timestamp"] = 1700000000;

    TEST_ASSERT_EQUAL_STRING("fail", doc["status"].as<const char *>());
    TEST_ASSERT_TRUE(doc["data"].is<JsonObject>());
    TEST_ASSERT_EQUAL_STRING(
        "speed out of range (0-255)",
        doc["data"]["failreason"].as<const char *>()
    );
}

void test_jsend_error_format(void) {
    JsonDocument doc;
    doc["status"] = "error";
    doc["message"] = "I2C command failed";
    doc["timestamp"] = 1700000000;

    TEST_ASSERT_EQUAL_STRING("error", doc["status"].as<const char *>());
    TEST_ASSERT_EQUAL_STRING("I2C command failed", doc["message"].as<const char *>());
    // Error format has message at top level, not in data
    TEST_ASSERT_TRUE(doc["data"].isNull());
}

void test_jsend_success_serialization(void) {
    JsonDocument doc;
    doc["status"] = "success";
    doc["data"]["value"] = 123;

    char buf[256];
    serializeJson(doc, buf, sizeof(buf));

    // Parse back and verify round-trip
    JsonDocument parsed;
    deserializeJson(parsed, buf);
    TEST_ASSERT_EQUAL_STRING("success", parsed["status"].as<const char *>());
    TEST_ASSERT_EQUAL(123, parsed["data"]["value"].as<int>());
}

// ---------------------------------------------------------------------------
// Command enum validation
// ---------------------------------------------------------------------------

static const char *VALID_COMMANDS[] = {
    "low", "medium", "high",
    "timer1", "timer2", "timer3",
    "away", "auto", "autonight",
    "cook30", "cook60",
    "motion_on", "motion_off",
    "join", "leave",
    "clearqueue",
    nullptr
};

static bool isValidCommand(const char *cmd) {
    for (int i = 0; VALID_COMMANDS[i] != nullptr; i++) {
        if (strcmp(cmd, VALID_COMMANDS[i]) == 0)
            return true;
    }
    return false;
}

void test_valid_commands_accepted(void) {
    TEST_ASSERT_TRUE(isValidCommand("low"));
    TEST_ASSERT_TRUE(isValidCommand("medium"));
    TEST_ASSERT_TRUE(isValidCommand("high"));
    TEST_ASSERT_TRUE(isValidCommand("timer1"));
    TEST_ASSERT_TRUE(isValidCommand("timer2"));
    TEST_ASSERT_TRUE(isValidCommand("timer3"));
    TEST_ASSERT_TRUE(isValidCommand("away"));
    TEST_ASSERT_TRUE(isValidCommand("auto"));
    TEST_ASSERT_TRUE(isValidCommand("autonight"));
    TEST_ASSERT_TRUE(isValidCommand("cook30"));
    TEST_ASSERT_TRUE(isValidCommand("cook60"));
    TEST_ASSERT_TRUE(isValidCommand("clearqueue"));
    TEST_ASSERT_TRUE(isValidCommand("join"));
    TEST_ASSERT_TRUE(isValidCommand("leave"));
}

void test_invalid_commands_rejected(void) {
    TEST_ASSERT_FALSE(isValidCommand("nonexistent"));
    TEST_ASSERT_FALSE(isValidCommand(""));
    TEST_ASSERT_FALSE(isValidCommand("LOW"));  // case sensitive
    TEST_ASSERT_FALSE(isValidCommand("reboot"));
    TEST_ASSERT_FALSE(isValidCommand("speed"));
}

// ---------------------------------------------------------------------------
// Parameter range validation
// ---------------------------------------------------------------------------

static bool isSpeedInRange(int speed) {
    return speed >= 0 && speed <= 255;
}

static bool isTimerInRange(int timer) {
    return timer >= 0 && timer <= 65535;
}

static bool isCO2InRange(int co2) {
    return co2 >= 0 && co2 <= 10000;
}

static bool isDemandInRange(int demand) {
    return demand >= 0 && demand <= 200;
}

void test_speed_range_valid(void) {
    TEST_ASSERT_TRUE(isSpeedInRange(0));
    TEST_ASSERT_TRUE(isSpeedInRange(128));
    TEST_ASSERT_TRUE(isSpeedInRange(255));
}

void test_speed_range_invalid(void) {
    TEST_ASSERT_FALSE(isSpeedInRange(-1));
    TEST_ASSERT_FALSE(isSpeedInRange(256));
    TEST_ASSERT_FALSE(isSpeedInRange(300));
    TEST_ASSERT_FALSE(isSpeedInRange(1000));
}

void test_timer_range_valid(void) {
    TEST_ASSERT_TRUE(isTimerInRange(0));
    TEST_ASSERT_TRUE(isTimerInRange(1));
    TEST_ASSERT_TRUE(isTimerInRange(65535));
}

void test_timer_range_invalid(void) {
    TEST_ASSERT_FALSE(isTimerInRange(-1));
    TEST_ASSERT_FALSE(isTimerInRange(65536));
    TEST_ASSERT_FALSE(isTimerInRange(70000));
}

void test_co2_range_valid(void) {
    TEST_ASSERT_TRUE(isCO2InRange(0));
    TEST_ASSERT_TRUE(isCO2InRange(400));
    TEST_ASSERT_TRUE(isCO2InRange(800));
    TEST_ASSERT_TRUE(isCO2InRange(10000));
}

void test_co2_range_invalid(void) {
    TEST_ASSERT_FALSE(isCO2InRange(-1));
    TEST_ASSERT_FALSE(isCO2InRange(10001));
    TEST_ASSERT_FALSE(isCO2InRange(20000));
}

void test_demand_range_valid(void) {
    TEST_ASSERT_TRUE(isDemandInRange(0));
    TEST_ASSERT_TRUE(isDemandInRange(100));
    TEST_ASSERT_TRUE(isDemandInRange(200));
}

void test_demand_range_invalid(void) {
    TEST_ASSERT_FALSE(isDemandInRange(-1));
    TEST_ASSERT_FALSE(isDemandInRange(201));
    TEST_ASSERT_FALSE(isDemandInRange(255));
}

// ---------------------------------------------------------------------------
// JSON body parsing edge cases
// ---------------------------------------------------------------------------

void test_parse_command_from_json(void) {
    const char *input = R"({"command":"low"})";
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, input);
    TEST_ASSERT_TRUE(err == DeserializationError::Ok);
    TEST_ASSERT_EQUAL_STRING("low", doc["command"].as<const char *>());
}

void test_parse_speed_timer_from_json(void) {
    const char *input = R"({"speed":150,"timer":10})";
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, input);
    TEST_ASSERT_TRUE(err == DeserializationError::Ok);
    TEST_ASSERT_EQUAL(150, doc["speed"].as<int>());
    TEST_ASSERT_EQUAL(10, doc["timer"].as<int>());
}

void test_parse_empty_json(void) {
    const char *input = "{}";
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, input);
    TEST_ASSERT_TRUE(err == DeserializationError::Ok);
    TEST_ASSERT_TRUE(doc["command"].isNull());
    TEST_ASSERT_TRUE(doc["speed"].isNull());
}

void test_parse_invalid_json(void) {
    const char *input = "{invalid json!!";
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, input);
    TEST_ASSERT_NOT_EQUAL(DeserializationError::Ok, err);
}

void test_containsKey_check(void) {
    const char *input = R"({"speed":100})";
    JsonDocument doc;
    deserializeJson(doc, input);
    JsonObject obj = doc.as<JsonObject>();
    TEST_ASSERT_FALSE(obj["speed"].isNull());
    TEST_ASSERT_TRUE(obj["command"].isNull());
    TEST_ASSERT_TRUE(obj["timer"].isNull());
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // JSend format
    RUN_TEST(test_jsend_success_format);
    RUN_TEST(test_jsend_fail_format);
    RUN_TEST(test_jsend_error_format);
    RUN_TEST(test_jsend_success_serialization);

    // Command validation
    RUN_TEST(test_valid_commands_accepted);
    RUN_TEST(test_invalid_commands_rejected);

    // Parameter ranges
    RUN_TEST(test_speed_range_valid);
    RUN_TEST(test_speed_range_invalid);
    RUN_TEST(test_timer_range_valid);
    RUN_TEST(test_timer_range_invalid);
    RUN_TEST(test_co2_range_valid);
    RUN_TEST(test_co2_range_invalid);
    RUN_TEST(test_demand_range_valid);
    RUN_TEST(test_demand_range_invalid);

    // JSON parsing
    RUN_TEST(test_parse_command_from_json);
    RUN_TEST(test_parse_speed_timer_from_json);
    RUN_TEST(test_parse_empty_json);
    RUN_TEST(test_parse_invalid_json);
    RUN_TEST(test_containsKey_check);

    return UNITY_END();
}
