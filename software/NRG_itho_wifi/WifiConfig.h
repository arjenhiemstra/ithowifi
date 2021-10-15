#pragma once

#define WIFI_CONFIG_VERSION "001"

#include "hardware.h"
#include <stdio.h>
#include <string.h>
#include <Arduino.h>
#include <ArduinoJson.h>

class WifiConfig {
  private:
  public:
    char ssid[33];
    char passwd[65];
    char dhcp[5];
    uint8_t renew;
    char ip[16];
    char subnet[16];
    char gateway[16];
    char dns1[16];
    char dns2[16];
    uint8_t port;
    char hostname[32];
    char config_struct_version[4];

    WifiConfig();
    ~WifiConfig();

    bool set(JsonObjectConst);
    void get(JsonObject) const;
  protected:


}; //WifiConfig
