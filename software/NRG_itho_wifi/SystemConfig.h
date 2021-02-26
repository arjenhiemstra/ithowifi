#ifndef SystemConfig_h
#define SystemConfig_h

#define CONFIG_VERSION "003" //Change when SystemConfig struc changes

#include <stdio.h>
#include <string.h>
#include <Arduino.h>
#include <ArduinoJson.h>

class SystemConfig {
  private:
  public:
    char sys_username[22];
    char sys_password[22];
    char syssec_web[5];
    char syssec_api[5];
    char syssec_edit[5];
    char mqtt_active[5];
    char mqtt_serverName[65];
    char mqtt_username[32];
    char mqtt_password[32];
    int mqtt_port;
    int mqtt_version;
    char mqtt_state_topic[128];
    char mqtt_state_retain[5];
    char mqtt_cmd_topic[128];
    char mqtt_lwt_topic[128];
    char mqtt_domoticz_active[4];
    uint16_t mqtt_idx;
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
    char itho_rf_support[5];
    mutable bool rfInitOK;
    char nonQ_cmd_clearsQ[5];
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


#endif //SystemConfig_h
