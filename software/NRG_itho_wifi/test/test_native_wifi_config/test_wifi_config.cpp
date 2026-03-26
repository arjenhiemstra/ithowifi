/**
 * Native unit tests for WifiConfig serialization logic.
 *
 * Reimplements a minimal WifiConfig struct and its set/get methods
 * based on config/WifiConfig.cpp to test serialization roundtrips,
 * default values, buffer overflow protection, and password handling.
 *
 * Build: pio test -e native_test --filter test_native_wifi_config
 */
#include <cstring>
#include <cstdint>
#include <unity.h>
#include <ArduinoJson.h>

#include "mocks/Arduino.h"

// ---------------------------------------------------------------------------
// Reimplemented WifiConfig from config/WifiConfig.cpp
// Field types and sizes match the real struct exactly.
// ---------------------------------------------------------------------------

struct WifiConfig {
    char ssid[33];
    char passwd[65];
    char appasswd[65];
    char dhcp[5];        // "on" or "off" -- real code uses char[5], not uint8_t
    char ip[16];
    char subnet[16];
    char gateway[16];
    char dns1[16];
    char dns2[16];
    uint8_t port;        // real code: uint8_t
    char hostname[32];
    char ntpserver[128]; // real code: char[128]
    char timezone[30];
    uint8_t aptimeout;

    WifiConfig() {
        strlcpy(ssid, "", sizeof(ssid));
        strlcpy(passwd, "", sizeof(passwd));
        strlcpy(appasswd, "password", sizeof(appasswd));
        strlcpy(dhcp, "on", sizeof(dhcp));
        strlcpy(ip, "192.168.4.1", sizeof(ip));
        strlcpy(subnet, "255.255.255.0", sizeof(subnet));
        strlcpy(gateway, "127.0.0.1", sizeof(gateway));
        strlcpy(dns1, "8.8.8.8", sizeof(dns1));
        strlcpy(dns2, "8.8.4.4", sizeof(dns2));
        port = 80;
        strlcpy(hostname, "", sizeof(hostname));
        strlcpy(ntpserver, "pool.ntp.org", sizeof(ntpserver));
        strlcpy(timezone, "Europe/Amsterdam", sizeof(timezone));
        aptimeout = 15;
    }

    bool set(JsonObject obj) {
        bool updated = false;
        if (!obj["ssid"].isNull()) {
            updated = true;
            strlcpy(ssid, obj["ssid"] | "", sizeof(ssid));
        }
        if (!obj["passwd"].isNull()) {
            const char *new_password = obj["passwd"] | "";
            // SECURITY: Ignore placeholder, only update if actual new password
            if (strcmp(new_password, "********") != 0 && strlen(new_password) > 0) {
                updated = true;
                strlcpy(passwd, new_password, sizeof(passwd));
            } else if (strlen(new_password) == 0) {
                updated = true;
                memset(passwd, 0, sizeof(passwd));
            }
        }
        if (!obj["appasswd"].isNull()) {
            const char *new_password = obj["appasswd"] | "";
            if (strcmp(new_password, "********") != 0 && strlen(new_password) > 0) {
                updated = true;
                strlcpy(appasswd, new_password, sizeof(appasswd));
            } else if (strlen(new_password) == 0) {
                updated = true;
                memset(appasswd, 0, sizeof(appasswd));
            }
        }
        if (!obj["dhcp"].isNull()) {
            updated = true;
            strlcpy(dhcp, obj["dhcp"] | "", sizeof(dhcp));
        }
        if (!obj["ip"].isNull()) {
            updated = true;
            strlcpy(ip, obj["ip"] | "", sizeof(ip));
        }
        if (!obj["subnet"].isNull()) {
            updated = true;
            strlcpy(subnet, obj["subnet"] | "", sizeof(subnet));
        }
        if (!obj["gateway"].isNull()) {
            updated = true;
            strlcpy(gateway, obj["gateway"] | "", sizeof(gateway));
        }
        if (!obj["dns1"].isNull()) {
            updated = true;
            strlcpy(dns1, obj["dns1"] | "", sizeof(dns1));
        }
        if (!obj["dns2"].isNull()) {
            updated = true;
            strlcpy(dns2, obj["dns2"] | "", sizeof(dns2));
        }
        if (!obj["port"].isNull()) {
            updated = true;
            port = obj["port"];
        }
        if (!obj["hostname"].isNull()) {
            updated = true;
            strlcpy(hostname, obj["hostname"] | "", sizeof(hostname));
        }
        if (!obj["ntpserver"].isNull()) {
            updated = true;
            strlcpy(ntpserver, obj["ntpserver"] | "", sizeof(ntpserver));
        }
        if (!obj["timezone"].isNull()) {
            updated = true;
            strlcpy(timezone, obj["timezone"] | "", sizeof(timezone));
        }
        if (!obj["aptimeout"].isNull()) {
            updated = true;
            aptimeout = obj["aptimeout"];
        }
        return updated;
    }

    void get(JsonObject obj) const {
        obj["ssid"] = ssid;
        obj["passwd"] = passwd;
        obj["appasswd"] = appasswd;
        obj["dhcp"] = dhcp;
        obj["ip"] = ip;
        obj["subnet"] = subnet;
        obj["gateway"] = gateway;
        obj["dns1"] = dns1;
        obj["dns2"] = dns2;
        obj["port"] = port;
        obj["hostname"] = hostname;
        obj["ntpserver"] = ntpserver;
        obj["timezone"] = timezone;
        obj["aptimeout"] = aptimeout;
    }
};

static WifiConfig cfg;

void setUp(void) { cfg = WifiConfig(); }
void tearDown(void) {}

// ---------------------------------------------------------------------------
// Default value tests
// ---------------------------------------------------------------------------

void test_default_ssid_empty(void) { TEST_ASSERT_EQUAL_STRING("", cfg.ssid); }
void test_default_passwd_empty(void) { TEST_ASSERT_EQUAL_STRING("", cfg.passwd); }
void test_default_appasswd(void) { TEST_ASSERT_EQUAL_STRING("password", cfg.appasswd); }
void test_default_dhcp_on(void) { TEST_ASSERT_EQUAL_STRING("on", cfg.dhcp); }
void test_default_ip(void) { TEST_ASSERT_EQUAL_STRING("192.168.4.1", cfg.ip); }
void test_default_subnet(void) { TEST_ASSERT_EQUAL_STRING("255.255.255.0", cfg.subnet); }
void test_default_gateway(void) { TEST_ASSERT_EQUAL_STRING("127.0.0.1", cfg.gateway); }
void test_default_dns1(void) { TEST_ASSERT_EQUAL_STRING("8.8.8.8", cfg.dns1); }
void test_default_dns2(void) { TEST_ASSERT_EQUAL_STRING("8.8.4.4", cfg.dns2); }
void test_default_port(void) { TEST_ASSERT_EQUAL(80, cfg.port); }
void test_default_hostname_empty(void) { TEST_ASSERT_EQUAL_STRING("", cfg.hostname); }
void test_default_ntpserver(void) { TEST_ASSERT_EQUAL_STRING("pool.ntp.org", cfg.ntpserver); }
void test_default_timezone(void) { TEST_ASSERT_EQUAL_STRING("Europe/Amsterdam", cfg.timezone); }
void test_default_aptimeout(void) { TEST_ASSERT_EQUAL(15, cfg.aptimeout); }

// ---------------------------------------------------------------------------
// set() single field tests
// ---------------------------------------------------------------------------

void test_set_hostname(void) {
    JsonDocument doc;
    doc["hostname"] = "mydevice";
    TEST_ASSERT_TRUE(cfg.set(doc.as<JsonObject>()));
    TEST_ASSERT_EQUAL_STRING("mydevice", cfg.hostname);
}

void test_set_ssid(void) {
    JsonDocument doc;
    doc["ssid"] = "MyWiFi";
    cfg.set(doc.as<JsonObject>());
    TEST_ASSERT_EQUAL_STRING("MyWiFi", cfg.ssid);
}

void test_set_port(void) {
    JsonDocument doc;
    doc["port"] = 80;
    cfg.set(doc.as<JsonObject>());
    TEST_ASSERT_EQUAL(80, cfg.port);
}

void test_set_empty_returns_false(void) {
    JsonDocument doc;
    TEST_ASSERT_FALSE(cfg.set(doc.as<JsonObject>()));
}

// ---------------------------------------------------------------------------
// set() multiple fields
// ---------------------------------------------------------------------------

void test_set_multiple_fields(void) {
    JsonDocument doc;
    doc["ssid"] = "TestNet";
    doc["dhcp"] = "off";
    doc["ip"] = "10.0.0.50";
    doc["timezone"] = "Europe/Berlin";
    bool updated = cfg.set(doc.as<JsonObject>());
    TEST_ASSERT_TRUE(updated);
    TEST_ASSERT_EQUAL_STRING("TestNet", cfg.ssid);
    TEST_ASSERT_EQUAL_STRING("off", cfg.dhcp);
    TEST_ASSERT_EQUAL_STRING("10.0.0.50", cfg.ip);
    TEST_ASSERT_EQUAL_STRING("Europe/Berlin", cfg.timezone);
    // Unchanged fields retain defaults
    TEST_ASSERT_EQUAL_STRING("255.255.255.0", cfg.subnet);
    TEST_ASSERT_EQUAL_STRING("pool.ntp.org", cfg.ntpserver);
}

void test_set_static_ip_config(void) {
    JsonDocument doc;
    doc["dhcp"] = "off";
    doc["ip"] = "192.168.1.100";
    doc["subnet"] = "255.255.255.0";
    doc["gateway"] = "192.168.1.1";
    doc["dns1"] = "1.1.1.1";
    doc["dns2"] = "1.0.0.1";
    cfg.set(doc.as<JsonObject>());
    TEST_ASSERT_EQUAL_STRING("off", cfg.dhcp);
    TEST_ASSERT_EQUAL_STRING("192.168.1.100", cfg.ip);
    TEST_ASSERT_EQUAL_STRING("255.255.255.0", cfg.subnet);
    TEST_ASSERT_EQUAL_STRING("192.168.1.1", cfg.gateway);
    TEST_ASSERT_EQUAL_STRING("1.1.1.1", cfg.dns1);
    TEST_ASSERT_EQUAL_STRING("1.0.0.1", cfg.dns2);
}

// ---------------------------------------------------------------------------
// get() tests
// ---------------------------------------------------------------------------

void test_get_all_keys_present(void) {
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    cfg.get(obj);
    TEST_ASSERT_FALSE(obj["ssid"].isNull());
    TEST_ASSERT_FALSE(obj["passwd"].isNull());
    TEST_ASSERT_FALSE(obj["appasswd"].isNull());
    TEST_ASSERT_FALSE(obj["dhcp"].isNull());
    TEST_ASSERT_FALSE(obj["ip"].isNull());
    TEST_ASSERT_FALSE(obj["subnet"].isNull());
    TEST_ASSERT_FALSE(obj["gateway"].isNull());
    TEST_ASSERT_FALSE(obj["dns1"].isNull());
    TEST_ASSERT_FALSE(obj["dns2"].isNull());
    TEST_ASSERT_FALSE(obj["port"].isNull());
    TEST_ASSERT_FALSE(obj["hostname"].isNull());
    TEST_ASSERT_FALSE(obj["ntpserver"].isNull());
    TEST_ASSERT_FALSE(obj["timezone"].isNull());
    TEST_ASSERT_FALSE(obj["aptimeout"].isNull());
}

void test_get_default_values(void) {
    JsonDocument doc;
    JsonObject obj = doc.to<JsonObject>();
    cfg.get(obj);
    TEST_ASSERT_EQUAL_STRING("", obj["ssid"].as<const char *>());
    TEST_ASSERT_EQUAL(80, obj["port"].as<int>());
    TEST_ASSERT_EQUAL_STRING("pool.ntp.org", obj["ntpserver"].as<const char *>());
    TEST_ASSERT_EQUAL_STRING("Europe/Amsterdam", obj["timezone"].as<const char *>());
    TEST_ASSERT_EQUAL(15, obj["aptimeout"].as<int>());
}

// ---------------------------------------------------------------------------
// Roundtrip tests
// ---------------------------------------------------------------------------

void test_set_get_roundtrip(void) {
    JsonDocument setDoc;
    setDoc["hostname"] = "testhost";
    setDoc["ssid"] = "TestNet";
    setDoc["dhcp"] = "off";
    setDoc["ip"] = "10.0.0.1";
    setDoc["ntpserver"] = "time.google.com";
    setDoc["timezone"] = "Europe/London";
    setDoc["aptimeout"] = 30;
    cfg.set(setDoc.as<JsonObject>());

    JsonDocument getDoc;
    JsonObject obj = getDoc.to<JsonObject>();
    cfg.get(obj);
    TEST_ASSERT_EQUAL_STRING("testhost", obj["hostname"].as<const char *>());
    TEST_ASSERT_EQUAL_STRING("TestNet", obj["ssid"].as<const char *>());
    TEST_ASSERT_EQUAL_STRING("off", obj["dhcp"].as<const char *>());
    TEST_ASSERT_EQUAL_STRING("10.0.0.1", obj["ip"].as<const char *>());
    TEST_ASSERT_EQUAL_STRING("time.google.com", obj["ntpserver"].as<const char *>());
    TEST_ASSERT_EQUAL_STRING("Europe/London", obj["timezone"].as<const char *>());
    TEST_ASSERT_EQUAL(30, obj["aptimeout"].as<int>());
}

// ---------------------------------------------------------------------------
// Buffer overflow protection
// ---------------------------------------------------------------------------

void test_long_hostname_truncated(void) {
    JsonDocument doc;
    doc["hostname"] = "this-is-a-very-long-hostname-that-exceeds-the-buffer-size";
    cfg.set(doc.as<JsonObject>());
    TEST_ASSERT_EQUAL(31, strlen(cfg.hostname)); // char[32] -> max 31 chars + null
}

void test_long_ssid_truncated(void) {
    JsonDocument doc;
    doc["ssid"] = "this-ssid-is-way-too-long-for-the-32-char-buffer-limit-set-by-wifi";
    cfg.set(doc.as<JsonObject>());
    TEST_ASSERT_EQUAL(32, strlen(cfg.ssid)); // char[33] -> max 32 chars + null
}

// ---------------------------------------------------------------------------
// Password handling
// ---------------------------------------------------------------------------

void test_passwd_placeholder_ignored(void) {
    JsonDocument doc;
    doc["passwd"] = "secret123";
    cfg.set(doc.as<JsonObject>());
    TEST_ASSERT_EQUAL_STRING("secret123", cfg.passwd);

    JsonDocument doc2;
    doc2["passwd"] = "********";
    cfg.set(doc2.as<JsonObject>());
    TEST_ASSERT_EQUAL_STRING("secret123", cfg.passwd); // unchanged
}

void test_passwd_empty_clears(void) {
    JsonDocument doc;
    doc["passwd"] = "secret123";
    cfg.set(doc.as<JsonObject>());

    JsonDocument doc2;
    doc2["passwd"] = "";
    cfg.set(doc2.as<JsonObject>());
    TEST_ASSERT_EQUAL_STRING("", cfg.passwd);
}

void test_passwd_real_value_accepted(void) {
    JsonDocument doc;
    doc["passwd"] = "newpassword";
    cfg.set(doc.as<JsonObject>());
    TEST_ASSERT_EQUAL_STRING("newpassword", cfg.passwd);
}

void test_appasswd_placeholder_ignored(void) {
    // Default appasswd is "password"
    JsonDocument doc;
    doc["appasswd"] = "********";
    cfg.set(doc.as<JsonObject>());
    TEST_ASSERT_EQUAL_STRING("password", cfg.appasswd);
}

void test_appasswd_updated(void) {
    JsonDocument doc;
    doc["appasswd"] = "newsecret";
    cfg.set(doc.as<JsonObject>());
    TEST_ASSERT_EQUAL_STRING("newsecret", cfg.appasswd);
}

void test_appasswd_empty_clears(void) {
    JsonDocument doc;
    doc["appasswd"] = "";
    cfg.set(doc.as<JsonObject>());
    TEST_ASSERT_EQUAL_STRING("", cfg.appasswd);
}

// ---------------------------------------------------------------------------
// Empty string handling
// ---------------------------------------------------------------------------

void test_set_empty_ssid(void) {
    JsonDocument doc;
    doc["ssid"] = "HasSSID";
    cfg.set(doc.as<JsonObject>());
    TEST_ASSERT_EQUAL_STRING("HasSSID", cfg.ssid);

    JsonDocument doc2;
    doc2["ssid"] = "";
    cfg.set(doc2.as<JsonObject>());
    TEST_ASSERT_EQUAL_STRING("", cfg.ssid);
}

// ---------------------------------------------------------------------------
// DHCP field
// ---------------------------------------------------------------------------

void test_dhcp_off(void) {
    JsonDocument doc;
    doc["dhcp"] = "off";
    cfg.set(doc.as<JsonObject>());
    TEST_ASSERT_EQUAL_STRING("off", cfg.dhcp);
}

void test_dhcp_on(void) {
    JsonDocument doc;
    doc["dhcp"] = "off";
    cfg.set(doc.as<JsonObject>());
    TEST_ASSERT_EQUAL_STRING("off", cfg.dhcp);

    JsonDocument doc2;
    doc2["dhcp"] = "on";
    cfg.set(doc2.as<JsonObject>());
    TEST_ASSERT_EQUAL_STRING("on", cfg.dhcp);
}

// ---------------------------------------------------------------------------
// Aptimeout
// ---------------------------------------------------------------------------

void test_set_aptimeout(void) {
    JsonDocument doc;
    doc["aptimeout"] = 30;
    cfg.set(doc.as<JsonObject>());
    TEST_ASSERT_EQUAL(30, cfg.aptimeout);
}

// ---------------------------------------------------------------------------
// get() includes password in output (masking is done in presentation layer)
// ---------------------------------------------------------------------------

void test_get_includes_password(void) {
    JsonDocument doc;
    doc["passwd"] = "secret123";
    cfg.set(doc.as<JsonObject>());

    JsonDocument getDoc;
    JsonObject obj = getDoc.to<JsonObject>();
    cfg.get(obj);
    // get() returns the actual password, masking is the responsibility of the caller
    TEST_ASSERT_EQUAL_STRING("secret123", obj["passwd"].as<const char *>());
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    UNITY_BEGIN();

    // Default values (14 tests)
    RUN_TEST(test_default_ssid_empty);
    RUN_TEST(test_default_passwd_empty);
    RUN_TEST(test_default_appasswd);
    RUN_TEST(test_default_dhcp_on);
    RUN_TEST(test_default_ip);
    RUN_TEST(test_default_subnet);
    RUN_TEST(test_default_gateway);
    RUN_TEST(test_default_dns1);
    RUN_TEST(test_default_dns2);
    RUN_TEST(test_default_port);
    RUN_TEST(test_default_hostname_empty);
    RUN_TEST(test_default_ntpserver);
    RUN_TEST(test_default_timezone);
    RUN_TEST(test_default_aptimeout);

    // Set single field (4 tests)
    RUN_TEST(test_set_hostname);
    RUN_TEST(test_set_ssid);
    RUN_TEST(test_set_port);
    RUN_TEST(test_set_empty_returns_false);

    // Set multiple fields (2 tests)
    RUN_TEST(test_set_multiple_fields);
    RUN_TEST(test_set_static_ip_config);

    // Get (2 tests)
    RUN_TEST(test_get_all_keys_present);
    RUN_TEST(test_get_default_values);

    // Roundtrip (1 test)
    RUN_TEST(test_set_get_roundtrip);

    // Buffer overflow (2 tests)
    RUN_TEST(test_long_hostname_truncated);
    RUN_TEST(test_long_ssid_truncated);

    // Password handling (6 tests)
    RUN_TEST(test_passwd_placeholder_ignored);
    RUN_TEST(test_passwd_empty_clears);
    RUN_TEST(test_passwd_real_value_accepted);
    RUN_TEST(test_appasswd_placeholder_ignored);
    RUN_TEST(test_appasswd_updated);
    RUN_TEST(test_appasswd_empty_clears);

    // Empty string and DHCP (3 tests)
    RUN_TEST(test_set_empty_ssid);
    RUN_TEST(test_dhcp_off);
    RUN_TEST(test_dhcp_on);

    // Aptimeout and password in get (2 tests)
    RUN_TEST(test_set_aptimeout);
    RUN_TEST(test_get_includes_password);

    return UNITY_END();
}
