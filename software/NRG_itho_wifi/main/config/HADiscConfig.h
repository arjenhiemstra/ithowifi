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

    char config_struct_version[4];

    mutable bool configLoaded;

    HADiscConfig();
    ~HADiscConfig();

    bool set(JsonObject);
    void get(JsonObject) const;
    void reset();

protected:
}; // HADiscConfig

extern HADiscConfig haDiscConfig;
