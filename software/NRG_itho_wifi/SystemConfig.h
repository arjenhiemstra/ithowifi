#pragma once

#define CONFIG_VERSION "005" //Change when SystemConfig struc changes

#include <stdio.h>
#include <string.h>
#include <Arduino.h>
#include <ArduinoJson.h>

class SystemConfig {
  private:
  public:
    char sys_username[22];
    char sys_password[22];
    uint8_t syssec_web;
    uint8_t syssec_api;
    uint8_t syssec_edit;
    uint8_t api_normalize;
    uint8_t syssht30;
    uint8_t mqtt_active;
    char mqtt_serverName[65];
    char mqtt_username[32];
    char mqtt_password[32];
    int mqtt_port;
    int mqtt_version;
    char mqtt_state_topic[128];
    char mqtt_ithostatus_topic[128];
    char mqtt_remotesinfo_topic[128];
    char mqtt_lastcmd_topic[128];    
    char mqtt_ha_topic[128];
    char mqtt_state_retain[5];
    char mqtt_cmd_topic[128];
    char mqtt_lwt_topic[128];
    uint8_t mqtt_domoticz_active;
    uint8_t mqtt_ha_active;
    uint16_t mqtt_idx;
    uint16_t sensor_idx;
    mutable bool mqtt_updated;
    mutable bool get_mqtt_settings;
    mutable bool get_sys_settings;
    uint8_t itho_fallback;
    uint8_t itho_low;
    uint8_t itho_medium;
    uint8_t itho_high;
    uint16_t itho_timer1;
    uint16_t itho_timer2;
    uint16_t itho_timer3;
    uint16_t itho_updatefreq;
    uint8_t itho_sendjoin;
    uint8_t itho_forcemedium;
    uint8_t itho_vremoteapi;
    uint8_t itho_rf_support;
    mutable bool rfInitOK;
    uint8_t nonQ_cmd_clearsQ;
    mutable bool itho_updated;  
    mutable bool get_itho_settings;
    char config_struct_version[4];
    mutable bool configLoaded;

    SystemConfig();
    ~SystemConfig();
    
    bool set(JsonObjectConst);
    void get(JsonObject) const;
  protected:


}; //SystemConfig

extern SystemConfig systemConfig;
