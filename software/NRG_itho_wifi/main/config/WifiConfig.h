#pragma once

#define WIFI_CONFIG_VERSION "001"

#include <cstdio>
#include <string>

#include <Arduino.h>
#include <ArduinoJson.h>

#include "esp_wifi.h"
#include "WiFiClient.h"
#include "WiFi.h"

#include "notifyClients.h"


class WifiConfig
{
private:
  typedef struct
  {
    wl_status_t code;
    const char *msg;
  } wl_status_msg;

  static const wl_status_msg wl_status_msg_table[];
  static const char *wl_unknown_msg;
  
  typedef struct
  {
    const char *tzlabel;
    const uint8_t tzindex;
  } tzLabels;

  static const tzLabels tzlabels[];
  static const char tz_strings[][30];

public:
  char ssid[33];
  char passwd[65];
  char dhcp[5];
  char ip[16];
  char subnet[16];
  char gateway[16];
  char dns1[16];
  char dns2[16];
  uint8_t port;
  char hostname[32];
  char ntpserver[128];
  char timezone[30];
  char config_struct_version[4];

  mutable bool configLoaded;

  WifiConfig();
  ~WifiConfig();

  bool set(JsonObjectConst);
  void get(JsonObject) const;

  const char *wl_status_to_name(wl_status_t code) const;
  const char *getTimeZoneStr() const;

protected:
}; // WifiConfig

void wifiScan();
void logWifiInfo();

extern WifiConfig wifiConfig;
