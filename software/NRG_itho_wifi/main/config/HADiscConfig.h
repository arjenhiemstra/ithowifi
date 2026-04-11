#pragma once

#define LOG_CONFIG_VERSION "001"

#include <cstdio>
#include <string>

#include <Arduino.h>
#include <ArduinoJson.h>

class HADiscConfig
{
public:
    uint8_t sscnt;
    char d[32];
    JsonDocument itemsDoc;
    JsonDocument rfDoc;  // RF device sensor/fan selections
    JsonDocument vrDoc;  // Virtual remote fan selections

    char config_struct_version[4];

    mutable bool configLoaded;

    HADiscConfig();
    ~HADiscConfig();

    bool set(JsonObject);
    void get(JsonObject) const;
    void reset();

    // RF sensor helpers
    static const char* getAvailableSensorsForType(uint16_t remoteType);
    static bool isSensorAvailableForType(uint16_t remoteType, const char* sensor);

    // RF fan presets helper (human-readable, used by HA Auto Discovery)
    static const char* getPresetsForType(uint16_t remoteType);
    // RF fan presets helper (wire format, used by external API consumers)
    static const char* getWirePresetsForType(uint16_t remoteType);

protected:
}; // HADiscConfig

extern HADiscConfig haDiscConfig;
