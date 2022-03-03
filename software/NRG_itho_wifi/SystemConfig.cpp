#include <string>

#include <Arduino.h>

#include "SystemConfig.h"

SystemConfig systemConfig;

// default constructor
SystemConfig::SystemConfig() {
  //default config
  strlcpy(config_struct_version, CONFIG_VERSION, sizeof(config_struct_version));
  strlcpy(sys_username, "admin", sizeof(sys_username));
  strlcpy(sys_password, "admin", sizeof(sys_password));    
  syssec_web = 0;
  syssec_api = 0;
  syssec_edit = 0;
  api_normalize = 0;
  syssht30 = 0;
  mqtt_active = 0;
  strlcpy(mqtt_serverName, "192.168.1.123", sizeof(mqtt_serverName));
  strlcpy(mqtt_username, "", sizeof(mqtt_username));
  strlcpy(mqtt_password, "", sizeof(mqtt_password));
  mqtt_port = 1883;
  mqtt_version = 1;
  strlcpy(mqtt_state_topic, "itho/state", sizeof(mqtt_state_topic));
  strlcpy(mqtt_ithostatus_topic, "itho/ithostatus", sizeof(mqtt_ithostatus_topic));
  strlcpy(mqtt_remotesinfo_topic, "itho/remotesinfo", sizeof(mqtt_remotesinfo_topic));
  strlcpy(mqtt_lastcmd_topic, "itho/lastcmd", sizeof(mqtt_lastcmd_topic));
  strlcpy(mqtt_ha_topic, "homeassistant", sizeof(mqtt_ha_topic));
  strlcpy(mqtt_state_retain, "yes", sizeof(mqtt_state_retain));
  strlcpy(mqtt_cmd_topic, "itho/cmd", sizeof(mqtt_cmd_topic));
  strlcpy(mqtt_lwt_topic, "itho/lwt", sizeof(mqtt_lwt_topic));
  mqtt_domoticz_active = 0;
  mqtt_ha_active = 0;
  mqtt_idx = 1;
  sensor_idx = 1;
  mqtt_updated = false;
  get_mqtt_settings = false;
  get_sys_settings = false;
  itho_fallback = 20;
  itho_low = 20;
  itho_medium = 120;
  itho_high = 220;
  itho_timer1 = 10;
  itho_timer2 = 20;
  itho_timer3 = 30;
  itho_numvrem = 1;
  itho_updatefreq = 10;
  itho_sendjoin = 0;
  itho_forcemedium = 0;
  itho_vremoteapi = 0;
  itho_rf_support = 0;
  rfInitOK = false;
  nonQ_cmd_clearsQ = 1;
  
  itho_updated = false;
  get_itho_settings = false;
  configLoaded = false;

} //SystemConfig

// default destructor
SystemConfig::~SystemConfig() {
} //~SystemConfig


bool SystemConfig::set(JsonObjectConst obj) {
  bool updated = false;

  if (!configLoaded) {
    if (obj["version_of_program"] != CONFIG_VERSION) {
      return false;
    }
  }
  if (!(const char*)obj["sys_username"].isNull()) {
    updated = true;
    strlcpy(sys_username, obj["sys_username"], sizeof(sys_username));
  }
  if (!(const char*)obj["sys_password"].isNull()) {
    updated = true;
    strlcpy(sys_password, obj["sys_password"], sizeof(sys_password));
  }
  if (!(const char*)obj["syssec_web"].isNull()) {
    updated = true;
    syssec_web = obj["syssec_web"];
  }  
  if (!(const char*)obj["syssec_api"].isNull()) {
    updated = true;
    syssec_api = obj["syssec_api"];
  }
  if (!(const char*)obj["syssec_edit"].isNull()) {
    updated = true;
    syssec_edit = obj["syssec_edit"];
  }
  if (!(const char*)obj["api_normalize"].isNull()) {
    updated = true;
    api_normalize = obj["api_normalize"];
  }
  if (!(const char*)obj["syssht30"].isNull()) {
    updated = true;
    unsigned char value = obj["syssht30"];
    if(value == 0 && syssht30 == 1) value = 2; //reset itho setting if applicable
    syssht30 = value;
  }
  //MQTT Settings parse
  if (!(const char*)obj["mqtt_active"].isNull()) {
    mqtt_updated = true;
    updated = true;
    mqtt_active = obj["mqtt_active"];
  }
  if (!(const char*)obj["mqtt_serverName"].isNull()) {
    updated = true;
    strlcpy(mqtt_serverName, obj["mqtt_serverName"], sizeof(mqtt_serverName));
  }
  if (!(const char*)obj["mqtt_username"].isNull()) {
    updated = true;
    strlcpy(mqtt_username, obj["mqtt_username"], sizeof(mqtt_username));
  }
  if (!(const char*)obj["mqtt_password"].isNull()) {
    updated = true;
    strlcpy(mqtt_password, obj["mqtt_password"], sizeof(mqtt_password));
  }
  if (!(const char*)obj["mqtt_port"].isNull()) {
    updated = true;
    mqtt_port = obj["mqtt_port"];
  }
  if (!(const char*)obj["mqtt_version"].isNull()) {
    updated = true;
    mqtt_version = obj["mqtt_version"];
  }
  if (!(const char*)obj["mqtt_state_topic"].isNull()) {
    updated = true;
    strlcpy(mqtt_state_topic, obj["mqtt_state_topic"], sizeof(mqtt_state_topic));
  }
  if (!(const char*)obj["mqtt_ithostatus_topic"].isNull()) {
    updated = true;
    strlcpy(mqtt_ithostatus_topic, obj["mqtt_ithostatus_topic"], sizeof(mqtt_ithostatus_topic));
  }
  if (!(const char*)obj["mqtt_remotesinfo_topic"].isNull()) {
    updated = true;
    strlcpy(mqtt_remotesinfo_topic, obj["mqtt_remotesinfo_topic"], sizeof(mqtt_remotesinfo_topic));
  }
  if (!(const char*)obj["mqtt_lastcmd_topic"].isNull()) {
    updated = true;
    strlcpy(mqtt_lastcmd_topic, obj["mqtt_lastcmd_topic"], sizeof(mqtt_lastcmd_topic));
  }  
  if (!(const char *)obj["mqtt_ha_topic"].isNull()) {
    updated = true;
    mqtt_updated = true;
    strlcpy(mqtt_ha_topic, obj["mqtt_ha_topic"], sizeof(mqtt_ha_topic));
  }
  if (!(const char*)obj["mqtt_state_retain"].isNull()) {
    updated = true;
    strlcpy(mqtt_state_retain, obj["mqtt_state_retain"], sizeof(mqtt_state_retain));
  }
  if (!(const char*)obj["mqtt_cmd_topic"].isNull()) {
    updated = true;
    strlcpy(mqtt_cmd_topic, obj["mqtt_cmd_topic"], sizeof(mqtt_cmd_topic));
  }
  if (!(const char*)obj["mqtt_lwt_topic"].isNull()) {
    updated = true;
    strlcpy(mqtt_lwt_topic, obj["mqtt_lwt_topic"], sizeof(mqtt_lwt_topic));
  }
  if (!(const char*)obj["mqtt_domoticz_active"].isNull()) {
    updated = true;
    mqtt_domoticz_active = obj["mqtt_domoticz_active"];
  }
  if (!(const char*)obj["mqtt_ha_active"].isNull()) {
    updated = true;
    mqtt_updated = true;
    mqtt_ha_active = obj["mqtt_ha_active"];
  }
  if (!(const char*)obj["mqtt_idx"].isNull()) {
    updated = true;
    mqtt_idx = obj["mqtt_idx"];
  }
  if (!(const char*)obj["sensor_idx"].isNull()) {
    updated = true;
    sensor_idx = obj["sensor_idx"];
  }
  if (!(const char*)obj["itho_fallback"].isNull()) {
    //itho_updated = true;
    updated = true;
    itho_fallback = obj["itho_fallback"];
  }  
  if (!(const char*)obj["itho_low"].isNull()) {
    //itho_updated = true;
    updated = true;
    itho_low = obj["itho_low"];
  }
  if (!(const char*)obj["itho_medium"].isNull()) {
    updated = true;
    itho_medium = obj["itho_medium"];
  }
  if (!(const char*)obj["itho_high"].isNull()) {
    updated = true;
    itho_high = obj["itho_high"];
  }
  if (!(const char*)obj["itho_timer1"].isNull()) {
    updated = true;
    itho_timer1 = obj["itho_timer1"];
  }
  if (!(const char*)obj["itho_timer2"].isNull()) {
    updated = true;
    itho_timer2 = obj["itho_timer2"];
  }
  if (!(const char*)obj["itho_timer3"].isNull()) {
    updated = true;
    itho_timer3 = obj["itho_timer3"];
  }
  if (!(const char*)obj["itho_updatefreq"].isNull()) {
    updated = true;
    itho_updatefreq = obj["itho_updatefreq"];
  }
  if (!(const char*)obj["itho_numvrem"].isNull()) {
    updated = true;
    itho_numvrem = obj["itho_numvrem"];
  }  
  if (!(const char*)obj["itho_sendjoin"].isNull()) {
    updated = true;
    itho_sendjoin = obj["itho_sendjoin"];
  }
  if (!(const char*)obj["itho_forcemedium"].isNull()) {
    updated = true;
    itho_forcemedium = obj["itho_forcemedium"];
  }
  if (!(const char*)obj["itho_vremoteapi"].isNull()) {
    updated = true;
    itho_vremoteapi = obj["itho_vremoteapi"];
  }
  if (!(const char*)obj["itho_rf_support"].isNull()) {
    updated = true;
    itho_rf_support = obj["itho_rf_support"];
  }
  if (!(const char*)obj["rfInitOK"].isNull()) {
    updated = true;
    rfInitOK = obj["rfInitOK"];
  }
  if (!(const char*)obj["nonQ_cmd_clearsQ"].isNull()) {
    updated = true;
    nonQ_cmd_clearsQ = obj["nonQ_cmd_clearsQ"];
  }    
  return updated;
}

void SystemConfig::get(JsonObject obj) const {

  bool complete = true;
  if (get_mqtt_settings || get_sys_settings || get_itho_settings) {
    complete = false;
  }
  if (complete || get_sys_settings) {
    get_sys_settings = false;
    obj["sys_username"] = sys_username;
    obj["sys_password"] = sys_password;
    obj["syssec_web"] = syssec_web;
    obj["syssec_api"] = syssec_api;
    obj["syssec_edit"] = syssec_edit;
    obj["api_normalize"] = api_normalize;
    obj["syssht30"] = syssht30;
    obj["itho_rf_support"] = itho_rf_support;
    obj["rfInitOK"] = rfInitOK;
    obj["itho_fallback"] = itho_fallback;
    obj["itho_low"] = itho_low;
    obj["itho_medium"] = itho_medium;
    obj["itho_high"] = itho_high;
    obj["itho_timer1"] = itho_timer1;
    obj["itho_timer2"] = itho_timer2;
    obj["itho_timer3"] = itho_timer3;
    obj["itho_updatefreq"] = itho_updatefreq;
    obj["itho_numvrem"] = itho_numvrem;
    obj["itho_sendjoin"] = itho_sendjoin;
    obj["itho_forcemedium"] = itho_forcemedium;
    obj["itho_vremoteapi"] = itho_vremoteapi;
    obj["nonQ_cmd_clearsQ"] = nonQ_cmd_clearsQ;    
  }
  if (complete || get_mqtt_settings) {
    get_mqtt_settings = false;
    obj["mqtt_active"] = mqtt_active;
    obj["mqtt_serverName"] = mqtt_serverName;
    obj["mqtt_username"] = mqtt_username;
    obj["mqtt_password"] = mqtt_password;
    obj["mqtt_port"] = mqtt_port;
    obj["mqtt_version"] = mqtt_version;
    obj["mqtt_state_topic"] = mqtt_state_topic;
    obj["mqtt_ithostatus_topic"] = mqtt_ithostatus_topic;
    obj["mqtt_remotesinfo_topic"] = mqtt_remotesinfo_topic;
    obj["mqtt_lastcmd_topic"] = mqtt_lastcmd_topic;
    obj["mqtt_state_retain"] = mqtt_state_retain;
    obj["mqtt_cmd_topic"] = mqtt_cmd_topic;
    obj["mqtt_lwt_topic"] = mqtt_lwt_topic;
    obj["mqtt_domoticz_active"] = mqtt_domoticz_active;
    obj["mqtt_ha_active"] = mqtt_ha_active;
    obj["mqtt_ha_topic"] = mqtt_ha_topic;
    obj["mqtt_idx"] = mqtt_idx;
    obj["sensor_idx"] = sensor_idx;
  }
  if (complete || get_itho_settings) {
    get_itho_settings = false;
  }
  obj["version_of_program"] = config_struct_version;
}
