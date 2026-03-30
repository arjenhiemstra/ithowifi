/**
 * Native unit tests for MQTT command JSON parsing.
 *
 * Tests that JSON values sent via MQTT are correctly parsed regardless
 * of whether they arrive as integers, strings, or other types.
 * This catches bugs like strtoul() on integer JSON variants (#350).
 *
 * Build: pio test -e native_test --filter test_native_mqtt_cmd_parse
 */
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <unity.h>
#include <ArduinoJson.h>

// ---------------------------------------------------------------------------
// Reimplemented parsing logic from MqttAPI.cpp mqttCallback()
// ---------------------------------------------------------------------------

// BROKEN: original code used strtoul on JsonVariant — crashes on integers
static uint8_t parseIndexBroken(JsonObject root, const char *key) {
    return strtoul(root[key], NULL, 10);
}

// FIXED: uses ArduinoJson .as<> which handles both types
static uint8_t parseIndexFixed(JsonObject root, const char *key) {
    return root[key].as<uint8_t>();
}

// Generic: parse a JSON value that could be int or string to uint8_t
static uint8_t parseJsonUint8(JsonVariant v) {
    return v.as<uint8_t>();
}

// Generic: parse a JSON value that could be int or string to uint16_t
static uint16_t parseJsonUint16(JsonVariant v) {
    return v.as<uint16_t>();
}

// Simulates the rfremotecmd block from mqttCallback
struct RfCommandResult {
    bool parsed;
    uint8_t idx;
    char cmd[32];
};

RfCommandResult parseRfRemoteCommand(const char *json) {
    RfCommandResult result = {false, 0, {}};
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json);
    if (error)
        return result;
    JsonObject root = doc.as<JsonObject>();

    if (!root["rfremotecmd"].isNull() || !root["rfremoteindex"].isNull()) {
        if (!root["rfremoteindex"].isNull()) {
            result.idx = root["rfremoteindex"].as<uint8_t>();
        }
        if (!root["rfremotecmd"].isNull()) {
            strlcpy(result.cmd, root["rfremotecmd"] | "", sizeof(result.cmd));
            result.parsed = true;
        }
    }
    return result;
}

// Simulates the rfco2 block from mqttCallback
struct RfCo2Result {
    bool parsed;
    uint8_t idx;
    uint16_t co2;
};

RfCo2Result parseRfCo2(const char *json) {
    RfCo2Result result = {false, 0, 0};
    JsonDocument doc;
    if (deserializeJson(doc, json))
        return result;
    JsonObject root = doc.as<JsonObject>();

    if (!root["rfco2"].isNull()) {
        if (!root["rfremoteindex"].isNull())
            result.idx = root["rfremoteindex"].as<uint8_t>();
        result.co2 = root["rfco2"].as<uint16_t>();
        result.parsed = true;
    }
    return result;
}

// Simulates the rfdemand block from mqttCallback
struct RfDemandResult {
    bool parsed;
    uint8_t idx;
    uint8_t demand;
    uint8_t zone;
};

RfDemandResult parseRfDemand(const char *json) {
    RfDemandResult result = {false, 0, 0, 0};
    JsonDocument doc;
    if (deserializeJson(doc, json))
        return result;
    JsonObject root = doc.as<JsonObject>();

    if (!root["rfdemand"].isNull()) {
        if (!root["rfremoteindex"].isNull())
            result.idx = root["rfremoteindex"].as<uint8_t>();
        if (!root["rfzone"].isNull())
            result.zone = root["rfzone"].as<uint8_t>();
        result.demand = root["rfdemand"].as<uint8_t>();
        result.parsed = true;
    }
    return result;
}

// ---------------------------------------------------------------------------
// Tests: JSON integer vs string type handling
// ---------------------------------------------------------------------------

void test_as_uint8_from_integer(void) {
    JsonDocument doc;
    deserializeJson(doc, "{\"val\": 5}");
    TEST_ASSERT_EQUAL(5, doc["val"].as<uint8_t>());
}

void test_as_uint8_from_string(void) {
    JsonDocument doc;
    deserializeJson(doc, "{\"val\": \"5\"}");
    TEST_ASSERT_EQUAL(5, doc["val"].as<uint8_t>());
}

void test_as_uint8_from_zero_integer(void) {
    JsonDocument doc;
    deserializeJson(doc, "{\"val\": 0}");
    TEST_ASSERT_EQUAL(0, doc["val"].as<uint8_t>());
}

void test_as_uint8_from_zero_string(void) {
    JsonDocument doc;
    deserializeJson(doc, "{\"val\": \"0\"}");
    TEST_ASSERT_EQUAL(0, doc["val"].as<uint8_t>());
}

void test_as_uint16_from_integer(void) {
    JsonDocument doc;
    deserializeJson(doc, "{\"val\": 1500}");
    TEST_ASSERT_EQUAL(1500, doc["val"].as<uint16_t>());
}

void test_as_uint16_from_string(void) {
    JsonDocument doc;
    deserializeJson(doc, "{\"val\": \"1500\"}");
    TEST_ASSERT_EQUAL(1500, doc["val"].as<uint16_t>());
}

// Verify that strtoul on a string variant works (this was always fine)
void test_strtoul_on_string_variant(void) {
    JsonDocument doc;
    deserializeJson(doc, "{\"val\": \"42\"}");
    // String variant converts to const char* correctly
    uint8_t val = strtoul(doc["val"], NULL, 10);
    TEST_ASSERT_EQUAL(42, val);
}

// Verify that strtoul on an integer variant is UNSAFE
// (returns 0 because as<const char*> returns nullptr for integers)
void test_strtoul_on_integer_variant_returns_zero_or_crashes(void) {
    JsonDocument doc;
    deserializeJson(doc, "{\"val\": 42}");
    // Integer variant: as<const char*> returns nullptr
    const char *str = doc["val"].as<const char *>();
    // On native x86 this returns nullptr (not a crash like on ESP32)
    // On ESP32 strtoul(nullptr, ...) causes a LoadProhibited exception
    if (str == nullptr) {
        TEST_PASS_MESSAGE("as<const char*> returns nullptr for integer — strtoul would crash on ESP32");
    } else {
        // If ArduinoJson converts it, check the value
        uint8_t val = strtoul(str, NULL, 10);
        TEST_ASSERT_EQUAL(42, val);
    }
}

// ---------------------------------------------------------------------------
// Tests: rfremotecmd parsing
// ---------------------------------------------------------------------------

void test_rfremote_integer_index(void) {
    auto r = parseRfRemoteCommand("{\"rfremotecmd\": \"high\", \"rfremoteindex\": 1}");
    TEST_ASSERT_TRUE(r.parsed);
    TEST_ASSERT_EQUAL(1, r.idx);
    TEST_ASSERT_EQUAL_STRING("high", r.cmd);
}

void test_rfremote_string_index(void) {
    auto r = parseRfRemoteCommand("{\"rfremotecmd\": \"high\", \"rfremoteindex\": \"1\"}");
    TEST_ASSERT_TRUE(r.parsed);
    TEST_ASSERT_EQUAL(1, r.idx);
    TEST_ASSERT_EQUAL_STRING("high", r.cmd);
}

void test_rfremote_zero_index_integer(void) {
    auto r = parseRfRemoteCommand("{\"rfremotecmd\": \"low\", \"rfremoteindex\": 0}");
    TEST_ASSERT_TRUE(r.parsed);
    TEST_ASSERT_EQUAL(0, r.idx);
    TEST_ASSERT_EQUAL_STRING("low", r.cmd);
}

void test_rfremote_zero_index_string(void) {
    auto r = parseRfRemoteCommand("{\"rfremotecmd\": \"low\", \"rfremoteindex\": \"0\"}");
    TEST_ASSERT_TRUE(r.parsed);
    TEST_ASSERT_EQUAL(0, r.idx);
}

void test_rfremote_no_index_defaults_zero(void) {
    auto r = parseRfRemoteCommand("{\"rfremotecmd\": \"auto\"}");
    TEST_ASSERT_TRUE(r.parsed);
    TEST_ASSERT_EQUAL(0, r.idx);
    TEST_ASSERT_EQUAL_STRING("auto", r.cmd);
}

void test_rfremote_only_index_no_cmd(void) {
    auto r = parseRfRemoteCommand("{\"rfremoteindex\": 3}");
    TEST_ASSERT_FALSE(r.parsed);
    TEST_ASSERT_EQUAL(3, r.idx);
}

void test_rfremote_various_commands(void) {
    const char *cmds[] = {"low", "medium", "high", "auto", "autonight", "timer1", "timer2", "timer3", "join", "leave"};
    for (size_t i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
        char json[128];
        snprintf(json, sizeof(json), "{\"rfremotecmd\": \"%s\", \"rfremoteindex\": 0}", cmds[i]);
        auto r = parseRfRemoteCommand(json);
        TEST_ASSERT_TRUE_MESSAGE(r.parsed, cmds[i]);
        TEST_ASSERT_EQUAL_STRING(cmds[i], r.cmd);
    }
}

// ---------------------------------------------------------------------------
// Tests: rfco2 parsing
// ---------------------------------------------------------------------------

void test_rfco2_integer_values(void) {
    auto r = parseRfCo2("{\"rfco2\": 800, \"rfremoteindex\": 1}");
    TEST_ASSERT_TRUE(r.parsed);
    TEST_ASSERT_EQUAL(800, r.co2);
    TEST_ASSERT_EQUAL(1, r.idx);
}

void test_rfco2_string_values(void) {
    auto r = parseRfCo2("{\"rfco2\": \"800\", \"rfremoteindex\": \"1\"}");
    TEST_ASSERT_TRUE(r.parsed);
    TEST_ASSERT_EQUAL(800, r.co2);
    TEST_ASSERT_EQUAL(1, r.idx);
}

void test_rfco2_no_index(void) {
    auto r = parseRfCo2("{\"rfco2\": 400}");
    TEST_ASSERT_TRUE(r.parsed);
    TEST_ASSERT_EQUAL(400, r.co2);
    TEST_ASSERT_EQUAL(0, r.idx);
}

// ---------------------------------------------------------------------------
// Tests: rfdemand parsing
// ---------------------------------------------------------------------------

void test_rfdemand_integer_values(void) {
    auto r = parseRfDemand("{\"rfdemand\": 150, \"rfremoteindex\": 2, \"rfzone\": 1}");
    TEST_ASSERT_TRUE(r.parsed);
    TEST_ASSERT_EQUAL(150, r.demand);
    TEST_ASSERT_EQUAL(2, r.idx);
    TEST_ASSERT_EQUAL(1, r.zone);
}

void test_rfdemand_string_values(void) {
    auto r = parseRfDemand("{\"rfdemand\": \"150\", \"rfremoteindex\": \"2\", \"rfzone\": \"1\"}");
    TEST_ASSERT_TRUE(r.parsed);
    TEST_ASSERT_EQUAL(150, r.demand);
    TEST_ASSERT_EQUAL(2, r.idx);
    TEST_ASSERT_EQUAL(1, r.zone);
}

void test_rfdemand_no_optional_fields(void) {
    auto r = parseRfDemand("{\"rfdemand\": 100}");
    TEST_ASSERT_TRUE(r.parsed);
    TEST_ASSERT_EQUAL(100, r.demand);
    TEST_ASSERT_EQUAL(0, r.idx);
    TEST_ASSERT_EQUAL(0, r.zone);
}

// ---------------------------------------------------------------------------
// Tests: mixed type payloads (real-world HA sends integers, mosquitto_pub strings)
// ---------------------------------------------------------------------------

void test_ha_style_payload(void) {
    // Home Assistant typically sends JSON with native types
    auto r = parseRfRemoteCommand("{\"rfremotecmd\":\"high\",\"rfremoteindex\":1}");
    TEST_ASSERT_TRUE(r.parsed);
    TEST_ASSERT_EQUAL(1, r.idx);
}

void test_mosquitto_pub_style_payload(void) {
    // mosquitto_pub with -m often sends string values
    auto r = parseRfRemoteCommand("{\"rfremotecmd\":\"high\",\"rfremoteindex\":\"1\"}");
    TEST_ASSERT_TRUE(r.parsed);
    TEST_ASSERT_EQUAL(1, r.idx);
}

void test_nodered_style_payload(void) {
    // Node-RED may send with whitespace
    auto r = parseRfRemoteCommand("{\n  \"rfremotecmd\": \"high\",\n  \"rfremoteindex\": 1\n}");
    TEST_ASSERT_TRUE(r.parsed);
    TEST_ASSERT_EQUAL(1, r.idx);
    TEST_ASSERT_EQUAL_STRING("high", r.cmd);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    UNITY_BEGIN();

    // JSON type handling
    RUN_TEST(test_as_uint8_from_integer);
    RUN_TEST(test_as_uint8_from_string);
    RUN_TEST(test_as_uint8_from_zero_integer);
    RUN_TEST(test_as_uint8_from_zero_string);
    RUN_TEST(test_as_uint16_from_integer);
    RUN_TEST(test_as_uint16_from_string);
    RUN_TEST(test_strtoul_on_string_variant);
    RUN_TEST(test_strtoul_on_integer_variant_returns_zero_or_crashes);

    // rfremotecmd
    RUN_TEST(test_rfremote_integer_index);
    RUN_TEST(test_rfremote_string_index);
    RUN_TEST(test_rfremote_zero_index_integer);
    RUN_TEST(test_rfremote_zero_index_string);
    RUN_TEST(test_rfremote_no_index_defaults_zero);
    RUN_TEST(test_rfremote_only_index_no_cmd);
    RUN_TEST(test_rfremote_various_commands);

    // rfco2
    RUN_TEST(test_rfco2_integer_values);
    RUN_TEST(test_rfco2_string_values);
    RUN_TEST(test_rfco2_no_index);

    // rfdemand
    RUN_TEST(test_rfdemand_integer_values);
    RUN_TEST(test_rfdemand_string_values);
    RUN_TEST(test_rfdemand_no_optional_fields);

    // Real-world payload formats
    RUN_TEST(test_ha_style_payload);
    RUN_TEST(test_mosquitto_pub_style_payload);
    RUN_TEST(test_nodered_style_payload);

    return UNITY_END();
}
