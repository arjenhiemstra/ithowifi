#ifndef SystemConfig_h
#define SystemConfig_h

#ifndef CONFIG_VERSION
#define CONFIG_VERSION "001"
#endif

#include <stdio.h>
#include <string.h>
#include <Arduino.h>
#include <ArduinoJson.h>

class SystemConfig {
  private:
  public:
    char mqtt_active[5];
    char mqtt_serverName[65];
    char mqtt_username[32];
    char mqtt_password[32];
    int mqtt_port;
    int mqtt_version;
    char mqtt_state_topic[128];
    char mqtt_state_retain[5];
    char mqtt_cmd_topic[128];
    char mqtt_domoticz_active[4];
    uint16_t mqtt_idx;
    uint8_t itho_low;
    uint8_t itho_medium;
    uint8_t itho_high;
    char config_struct_version[4];

    SystemConfig();
    ~SystemConfig();
    
    bool set(JsonObjectConst);
    void get(JsonObject) const;
  protected:


}; //SystemConfig


#endif //SystemConfig_h
