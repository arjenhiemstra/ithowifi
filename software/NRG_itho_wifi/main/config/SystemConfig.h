#pragma once

#define CONFIG_VERSION "005" // Change when SystemConfig struc changes

#include <cstdio>
#include <string>

#include <Arduino.h>
#include <ArduinoJson.h>

class SystemConfig
{
private:
public:
  char sys_username[33];
  char sys_password[33];
  uint8_t syssec_web;
  uint8_t syssec_api;
  uint8_t syssec_edit;
  uint8_t api_normalize;
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
  char mqtt_domoticzin_topic[128];
  char mqtt_domoticzout_topic[128];
  uint16_t mqtt_idx;
  uint16_t sensor_idx;
  uint8_t itho_fallback;
  uint8_t itho_low;
  uint8_t itho_medium;
  uint8_t itho_high;
  uint16_t itho_timer1;
  uint16_t itho_timer2;
  uint16_t itho_timer3;
  uint16_t itho_updatefreq;
  uint16_t itho_counter_updatefreq;
  uint8_t itho_numvrem;
  uint8_t itho_numrfrem;
  uint8_t itho_sendjoin;
  uint8_t itho_pwm2i2c;
  uint8_t itho_31da;
  uint8_t itho_31d9;
  uint8_t itho_2401;
  uint8_t itho_4210;
  uint8_t itho_forcemedium;
  uint8_t itho_vremoteapi;
  uint8_t itho_rf_support;
  uint8_t module_rf_id[3]{0, 0, 0};
  uint8_t i2cmenu;
  uint8_t i2c_safe_guard;
  uint8_t i2c_sniffer;
  uint8_t fw_check;
  uint8_t api_settings;
  uint8_t api_version;
  JsonDocument api_settings_activated;
  mutable bool rfInitOK;
  uint8_t nonQ_cmd_clearsQ;

  mutable bool itho_updated;
  mutable bool mqtt_updated;
  mutable bool get_mqtt_settings;
  mutable bool get_sys_settings;
  mutable bool get_rf_settings;

  char config_struct_version[4];
  mutable bool configLoaded;

  SystemConfig();
  ~SystemConfig();

  bool set(JsonObject);
  void get(JsonObject) const;

protected:
}; // SystemConfig

extern SystemConfig systemConfig;
