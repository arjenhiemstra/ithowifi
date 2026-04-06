/**
 * Native unit tests for MQTT response JSON format.
 *
 * Reimplements the response builder from MqttAPI.cpp mqttSendResponse()
 * to test JSON structure, field presence, and edge cases.
 *
 * Build: pio test -e native_test --filter test_native_mqtt_response
 */
#include <cstring>
#include <cstdint>
#include <ctime>
#include <unity.h>
#include <ArduinoJson.h>

// ---------------------------------------------------------------------------
// Reimplemented from MqttAPI.cpp mqttSendResponse()
// ---------------------------------------------------------------------------

void buildMqttResponse(char *buf, size_t bufSize, const char *status,
                       const char *command, const char *message,
                       time_t timestamp = 1711500000) {
    JsonDocument doc;
    doc["status"] = status;
    doc["command"] = command;
    if (message)
        doc["message"] = message;
    doc["timestamp"] = timestamp;
    serializeJson(doc, buf, bufSize);
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

void test_success_no_message(void) {
    char buf[256];
    buildMqttResponse(buf, sizeof(buf), "success", "low", nullptr);
    JsonDocument doc;
    TEST_ASSERT_TRUE(deserializeJson(doc, buf) == DeserializationError::Ok);
    TEST_ASSERT_EQUAL_STRING("success", doc["status"]);
    TEST_ASSERT_EQUAL_STRING("low", doc["command"]);
    TEST_ASSERT_TRUE(doc["message"].isNull());
    TEST_ASSERT_FALSE(doc["timestamp"].isNull());
}

void test_success_with_message(void) {
    char buf[256];
    buildMqttResponse(buf, sizeof(buf), "success", "speed", "set to 150");
    JsonDocument doc;
    deserializeJson(doc, buf);
    TEST_ASSERT_EQUAL_STRING("success", doc["status"]);
    TEST_ASSERT_EQUAL_STRING("speed", doc["command"]);
    TEST_ASSERT_EQUAL_STRING("set to 150", doc["message"]);
}

void test_fail_with_reason(void) {
    char buf[256];
    buildMqttResponse(buf, sizeof(buf), "fail", "rfco2", "remote must be RFT CO2 type");
    JsonDocument doc;
    deserializeJson(doc, buf);
    TEST_ASSERT_EQUAL_STRING("fail", doc["status"]);
    TEST_ASSERT_EQUAL_STRING("rfco2", doc["command"]);
    TEST_ASSERT_EQUAL_STRING("remote must be RFT CO2 type", doc["message"]);
}

void test_required_fields_present(void) {
    char buf[256];
    buildMqttResponse(buf, sizeof(buf), "success", "medium", nullptr);
    JsonDocument doc;
    deserializeJson(doc, buf);
    TEST_ASSERT_FALSE(doc["status"].isNull());
    TEST_ASSERT_FALSE(doc["command"].isNull());
    TEST_ASSERT_FALSE(doc["timestamp"].isNull());
}

void test_null_message_excluded(void) {
    char buf[256];
    buildMqttResponse(buf, sizeof(buf), "success", "high", nullptr);
    // The key "message" should not appear in serialized output
    TEST_ASSERT_NULL(strstr(buf, "message"));
}

void test_timestamp_value(void) {
    char buf[256];
    buildMqttResponse(buf, sizeof(buf), "success", "low", nullptr, 1711500000);
    JsonDocument doc;
    deserializeJson(doc, buf);
    TEST_ASSERT_EQUAL(1711500000, doc["timestamp"].as<long>());
}

void test_various_command_names(void) {
    const char *cmds[] = {"low", "medium", "high", "speed", "timer", "clearqueue",
                           "rfremotecmd", "rfco2", "rfdemand", "vremotecmd"};
    for (size_t i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
        char buf[256];
        buildMqttResponse(buf, sizeof(buf), "success", cmds[i], nullptr);
        JsonDocument doc;
        deserializeJson(doc, buf);
        TEST_ASSERT_EQUAL_STRING(cmds[i], doc["command"]);
    }
}

void test_special_chars_in_message(void) {
    char buf[256];
    buildMqttResponse(buf, sizeof(buf), "fail", "test", "error: \"quotes\" & <tags>");
    JsonDocument doc;
    TEST_ASSERT_TRUE(deserializeJson(doc, buf) == DeserializationError::Ok);
    TEST_ASSERT_EQUAL_STRING("error: \"quotes\" & <tags>", doc["message"]);
}

void test_small_buffer_truncation(void) {
    // ArduinoJson serializeJson truncates when buffer is too small.
    // A full response like {"status":"success","command":"low","timestamp":1711500000}
    // is about 60 bytes. With a 64-byte buffer, it gets cut off.
    char buf[64]{};
    buildMqttResponse(buf, sizeof(buf), "success", "low", nullptr);
    // The output is truncated but does not crash
    TEST_ASSERT_TRUE(strlen(buf) < 64);
    // The truncated output will not be valid JSON, which is expected
    // Just verify it wrote something or nothing without crashing
}

void test_valid_json_output(void) {
    char buf[256];
    buildMqttResponse(buf, sizeof(buf), "success", "clearqueue", nullptr);
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, buf);
    TEST_ASSERT_TRUE(err == DeserializationError::Ok);
}

void test_empty_command(void) {
    char buf[256];
    buildMqttResponse(buf, sizeof(buf), "success", "", nullptr);
    JsonDocument doc;
    deserializeJson(doc, buf);
    TEST_ASSERT_EQUAL_STRING("", doc["command"]);
}

void test_long_message(void) {
    char buf[512];
    const char *longMsg = "This is a very long error message that describes "
                          "in detail what went wrong with the command processing";
    buildMqttResponse(buf, sizeof(buf), "fail", "rfdemand", longMsg);
    JsonDocument doc;
    deserializeJson(doc, buf);
    TEST_ASSERT_EQUAL_STRING(longMsg, doc["message"]);
}

void test_field_order(void) {
    // Verify that status comes before command in the serialized output
    char buf[256];
    buildMqttResponse(buf, sizeof(buf), "success", "timer", nullptr);
    const char *statusPos = strstr(buf, "status");
    const char *commandPos = strstr(buf, "command");
    TEST_ASSERT_NOT_NULL(statusPos);
    TEST_ASSERT_NOT_NULL(commandPos);
    TEST_ASSERT_TRUE(statusPos < commandPos);
}

void test_error_status(void) {
    char buf[256];
    buildMqttResponse(buf, sizeof(buf), "error", "setsetting", "I2C command failed");
    JsonDocument doc;
    deserializeJson(doc, buf);
    TEST_ASSERT_EQUAL_STRING("error", doc["status"]);
    TEST_ASSERT_EQUAL_STRING("setsetting", doc["command"]);
    TEST_ASSERT_EQUAL_STRING("I2C command failed", doc["message"]);
}

void test_zero_timestamp(void) {
    char buf[256];
    buildMqttResponse(buf, sizeof(buf), "success", "low", nullptr, 0);
    JsonDocument doc;
    deserializeJson(doc, buf);
    TEST_ASSERT_EQUAL(0, doc["timestamp"].as<long>());
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    UNITY_BEGIN();

    RUN_TEST(test_success_no_message);
    RUN_TEST(test_success_with_message);
    RUN_TEST(test_fail_with_reason);
    RUN_TEST(test_required_fields_present);
    RUN_TEST(test_null_message_excluded);
    RUN_TEST(test_timestamp_value);
    RUN_TEST(test_various_command_names);
    RUN_TEST(test_special_chars_in_message);
    RUN_TEST(test_small_buffer_truncation);
    RUN_TEST(test_valid_json_output);
    RUN_TEST(test_empty_command);
    RUN_TEST(test_long_message);
    RUN_TEST(test_field_order);
    RUN_TEST(test_error_status);
    RUN_TEST(test_zero_timestamp);

    return UNITY_END();
}
