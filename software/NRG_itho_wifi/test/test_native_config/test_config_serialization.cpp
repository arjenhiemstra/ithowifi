/**
 * Native unit tests for config serialization logic.
 *
 * Tests SystemConfig default values and JSON set/get round-trip
 * without ESP32 hardware.
 *
 * Build: pio test -e native_test --filter test_native/test_config_serialization
 */
#include <unity.h>
#include <ArduinoJson.h>
#include <cstring>

// We cannot include the real SystemConfig.h because it pulls in Arduino.h
// and other ESP32 headers. Instead, we replicate the essential structure
// and default values here to test the serialization logic in isolation.

// ---------------------------------------------------------------------------
// Minimal SystemConfig replica for native testing
// ---------------------------------------------------------------------------

struct SystemConfigDefaults {
    char sys_username[33];
    char sys_password[33];
    uint8_t syssec_web;
    uint8_t syssec_api;
    uint8_t syssec_edit;
    uint8_t api_normalize;
    uint8_t api_settings;
    uint8_t api_reboot;
    uint8_t syssht30;
    uint8_t mqtt_active;
    char mqtt_serverName[65];
    char mqtt_username[33];
    char mqtt_password[33];
    int mqtt_port;
    int mqtt_version;
    char mqtt_base_topic[128];
    char mqtt_ha_topic[128];
    char mqtt_state_retain[5];
    uint8_t mqtt_domoticz_active;
    uint8_t mqtt_ha_active;
    uint8_t itho_fallback;
    uint8_t itho_low;
    uint8_t itho_medium;
    uint8_t itho_high;
    uint16_t itho_timer1;
    uint16_t itho_timer2;
    uint16_t itho_timer3;
    uint8_t itho_numvrem;
    uint8_t itho_numrfrem;
    uint8_t itho_pwm2i2c;
    uint8_t itho_rf_support;
    uint8_t fw_check;
    uint8_t nonQ_cmd_clearsQ;

    SystemConfigDefaults() {
        strlcpy(sys_username, "admin", sizeof(sys_username));
        strlcpy(sys_password, "admin", sizeof(sys_password));
        syssec_web = 0;
        syssec_api = 0;
        syssec_edit = 0;
        api_normalize = 0;
        api_settings = 0;
        api_reboot = 0;
        syssht30 = 0;
        mqtt_active = 0;
        strlcpy(mqtt_serverName, "192.168.1.123", sizeof(mqtt_serverName));
        strlcpy(mqtt_username, "", sizeof(mqtt_username));
        strlcpy(mqtt_password, "", sizeof(mqtt_password));
        mqtt_port = 1883;
        mqtt_version = 1;
        strlcpy(mqtt_base_topic, "itho", sizeof(mqtt_base_topic));
        strlcpy(mqtt_ha_topic, "homeassistant", sizeof(mqtt_ha_topic));
        strlcpy(mqtt_state_retain, "yes", sizeof(mqtt_state_retain));
        mqtt_domoticz_active = 0;
        mqtt_ha_active = 0;
        itho_fallback = 20;
        itho_low = 20;
        itho_medium = 120;
        itho_high = 220;
        itho_timer1 = 10;
        itho_timer2 = 20;
        itho_timer3 = 30;
        itho_numvrem = 1;
        itho_numrfrem = 10;
        itho_pwm2i2c = 1;
        itho_rf_support = 1;
        fw_check = 1;
        nonQ_cmd_clearsQ = 1;
    }

    void get(JsonObject obj) const {
        obj["sys_username"] = sys_username;
        obj["sys_password"] = sys_password;
        obj["syssec_web"] = syssec_web;
        obj["syssec_api"] = syssec_api;
        obj["syssec_edit"] = syssec_edit;
        obj["api_normalize"] = api_normalize;
        obj["api_settings"] = api_settings;
        obj["api_reboot"] = api_reboot;
        obj["syssht30"] = syssht30;
        obj["mqtt_active"] = mqtt_active;
        obj["mqtt_serverName"] = mqtt_serverName;
        obj["mqtt_username"] = mqtt_username;
        obj["mqtt_password"] = mqtt_password;
        obj["mqtt_port"] = mqtt_port;
        obj["mqtt_version"] = mqtt_version;
        obj["mqtt_base_topic"] = mqtt_base_topic;
        obj["mqtt_ha_topic"] = mqtt_ha_topic;
        obj["mqtt_state_retain"] = mqtt_state_retain;
        obj["mqtt_domoticz_active"] = mqtt_domoticz_active;
        obj["mqtt_ha_active"] = mqtt_ha_active;
        obj["itho_fallback"] = itho_fallback;
        obj["itho_low"] = itho_low;
        obj["itho_medium"] = itho_medium;
        obj["itho_high"] = itho_high;
        obj["itho_timer1"] = itho_timer1;
        obj["itho_timer2"] = itho_timer2;
        obj["itho_timer3"] = itho_timer3;
        obj["itho_numvrem"] = itho_numvrem;
        obj["itho_numrfrem"] = itho_numrfrem;
        obj["itho_pwm2i2c"] = itho_pwm2i2c;
        obj["itho_rf_support"] = itho_rf_support;
        obj["fw_check"] = fw_check;
        obj["nonQ_cmd_clearsQ"] = nonQ_cmd_clearsQ;
    }

    bool set(JsonObject obj) {
        bool updated = false;
        if (!obj["sys_username"].isNull()) { updated = true; strlcpy(sys_username, obj["sys_username"] | "", sizeof(sys_username)); }
        if (!obj["api_settings"].isNull()) { updated = true; api_settings = obj["api_settings"]; }
        if (!obj["api_reboot"].isNull()) { updated = true; api_reboot = obj["api_reboot"]; }
        if (!obj["mqtt_active"].isNull()) { updated = true; mqtt_active = obj["mqtt_active"]; }
        if (!obj["mqtt_port"].isNull()) { updated = true; mqtt_port = obj["mqtt_port"]; }
        if (!obj["itho_low"].isNull()) { updated = true; itho_low = obj["itho_low"]; }
        if (!obj["itho_medium"].isNull()) { updated = true; itho_medium = obj["itho_medium"]; }
        if (!obj["itho_high"].isNull()) { updated = true; itho_high = obj["itho_high"]; }
        if (!obj["sys_password"].isNull()) {
            const char* new_pw = obj["sys_password"] | "";
            if (strcmp(new_pw, "********") != 0 && strlen(new_pw) > 0) {
                updated = true;
                strlcpy(sys_password, new_pw, sizeof(sys_password));
            }
        }
        return updated;
    }
};

// ---------------------------------------------------------------------------
// Default value tests
// ---------------------------------------------------------------------------

void test_default_sys_username(void) {
    SystemConfigDefaults cfg;
    TEST_ASSERT_EQUAL_STRING("admin", cfg.sys_username);
}

void test_default_sys_password(void) {
    SystemConfigDefaults cfg;
    TEST_ASSERT_EQUAL_STRING("admin", cfg.sys_password);
}

void test_default_api_reboot_is_zero(void) {
    SystemConfigDefaults cfg;
    TEST_ASSERT_EQUAL_UINT8(0, cfg.api_reboot);
}

void test_default_api_settings_is_zero(void) {
    SystemConfigDefaults cfg;
    TEST_ASSERT_EQUAL_UINT8(0, cfg.api_settings);
}

void test_default_mqtt_port(void) {
    SystemConfigDefaults cfg;
    TEST_ASSERT_EQUAL_INT(1883, cfg.mqtt_port);
}

void test_default_itho_speeds(void) {
    SystemConfigDefaults cfg;
    TEST_ASSERT_EQUAL_UINT8(20, cfg.itho_low);
    TEST_ASSERT_EQUAL_UINT8(120, cfg.itho_medium);
    TEST_ASSERT_EQUAL_UINT8(220, cfg.itho_high);
}

void test_default_itho_timers(void) {
    SystemConfigDefaults cfg;
    TEST_ASSERT_EQUAL_UINT16(10, cfg.itho_timer1);
    TEST_ASSERT_EQUAL_UINT16(20, cfg.itho_timer2);
    TEST_ASSERT_EQUAL_UINT16(30, cfg.itho_timer3);
}

void test_default_mqtt_base_topic(void) {
    SystemConfigDefaults cfg;
    TEST_ASSERT_EQUAL_STRING("itho", cfg.mqtt_base_topic);
}

void test_default_ha_topic(void) {
    SystemConfigDefaults cfg;
    TEST_ASSERT_EQUAL_STRING("homeassistant", cfg.mqtt_ha_topic);
}

// ---------------------------------------------------------------------------
// api_version field no longer exists
// ---------------------------------------------------------------------------

void test_api_version_not_in_serialized_output(void) {
    SystemConfigDefaults cfg;
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    cfg.get(obj);

    // api_version was removed in v3, should not be present
    TEST_ASSERT_TRUE(doc["api_version"].isNull());
}

// ---------------------------------------------------------------------------
// Serialization round-trip tests
// ---------------------------------------------------------------------------

void test_get_produces_all_expected_keys(void) {
    SystemConfigDefaults cfg;
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    cfg.get(obj);

    TEST_ASSERT_FALSE(doc["sys_username"].isNull());
    TEST_ASSERT_FALSE(doc["api_reboot"].isNull());
    TEST_ASSERT_FALSE(doc["api_settings"].isNull());
    TEST_ASSERT_FALSE(doc["mqtt_active"].isNull());
    TEST_ASSERT_FALSE(doc["mqtt_port"].isNull());
    TEST_ASSERT_FALSE(doc["itho_low"].isNull());
    TEST_ASSERT_FALSE(doc["itho_medium"].isNull());
    TEST_ASSERT_FALSE(doc["itho_high"].isNull());
}

void test_set_updates_values(void) {
    SystemConfigDefaults cfg;

    JsonDocument doc;
    doc["api_reboot"] = 1;
    doc["itho_low"] = 50;
    doc["mqtt_port"] = 8883;

    bool updated = cfg.set(doc.as<JsonObject>());
    TEST_ASSERT_TRUE(updated);
    TEST_ASSERT_EQUAL_UINT8(1, cfg.api_reboot);
    TEST_ASSERT_EQUAL_UINT8(50, cfg.itho_low);
    TEST_ASSERT_EQUAL_INT(8883, cfg.mqtt_port);
}

void test_set_get_roundtrip(void) {
    SystemConfigDefaults cfg;

    // Modify some values
    cfg.api_reboot = 1;
    cfg.itho_low = 50;
    cfg.mqtt_port = 8883;

    // Serialize
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    cfg.get(obj);

    // Deserialize into a new config
    SystemConfigDefaults cfg2;
    cfg2.set(doc.as<JsonObject>());

    TEST_ASSERT_EQUAL_UINT8(1, cfg2.api_reboot);
    TEST_ASSERT_EQUAL_UINT8(50, cfg2.itho_low);
    TEST_ASSERT_EQUAL_INT(8883, cfg2.mqtt_port);
}

void test_set_with_no_keys_returns_false(void) {
    SystemConfigDefaults cfg;
    JsonDocument doc;
    // Empty object
    bool updated = cfg.set(doc.as<JsonObject>());
    TEST_ASSERT_FALSE(updated);
}

void test_password_placeholder_ignored(void) {
    SystemConfigDefaults cfg;

    // Set password to a known value
    strlcpy(cfg.sys_password, "secret123", sizeof(cfg.sys_password));

    // Sending the placeholder should NOT overwrite
    JsonDocument doc;
    doc["sys_password"] = "********";
    cfg.set(doc.as<JsonObject>());

    TEST_ASSERT_EQUAL_STRING("secret123", cfg.sys_password);
}

void test_password_real_value_updates(void) {
    SystemConfigDefaults cfg;

    JsonDocument doc;
    doc["sys_password"] = "newpassword";
    bool updated = cfg.set(doc.as<JsonObject>());

    TEST_ASSERT_TRUE(updated);
    TEST_ASSERT_EQUAL_STRING("newpassword", cfg.sys_password);
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main(int argc, char **argv) {
    UNITY_BEGIN();

    // Default values
    RUN_TEST(test_default_sys_username);
    RUN_TEST(test_default_sys_password);
    RUN_TEST(test_default_api_reboot_is_zero);
    RUN_TEST(test_default_api_settings_is_zero);
    RUN_TEST(test_default_mqtt_port);
    RUN_TEST(test_default_itho_speeds);
    RUN_TEST(test_default_itho_timers);
    RUN_TEST(test_default_mqtt_base_topic);
    RUN_TEST(test_default_ha_topic);

    // api_version removed
    RUN_TEST(test_api_version_not_in_serialized_output);

    // Serialization
    RUN_TEST(test_get_produces_all_expected_keys);
    RUN_TEST(test_set_updates_values);
    RUN_TEST(test_set_get_roundtrip);
    RUN_TEST(test_set_with_no_keys_returns_false);
    RUN_TEST(test_password_placeholder_ignored);
    RUN_TEST(test_password_real_value_updates);

    return UNITY_END();
}
