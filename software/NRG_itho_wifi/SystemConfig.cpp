#include "SystemConfig.h"
#include <string.h>
#include <Arduino.h>


// default constructor
SystemConfig::SystemConfig() {
  //default config
  strlcpy(config_struct_version, CONFIG_VERSION, sizeof(config_struct_version));
  strlcpy(sys_username, "admin", sizeof(sys_username));
  strlcpy(sys_password, "admin", sizeof(sys_password));    
  syssec_web = 0;
  syssec_api = 0;
  syssec_edit = 0;
  syssht30 = 0;
  mqtt_active = 0;
  strlcpy(mqtt_serverName, "192.168.1.123", sizeof(mqtt_serverName));
  strlcpy(mqtt_username, "", sizeof(mqtt_username));
  strlcpy(mqtt_password, "", sizeof(mqtt_password));
  mqtt_port = 1883;
  mqtt_version = 1;
  strlcpy(mqtt_state_topic, "itho/state", sizeof(mqtt_state_topic));
  strlcpy(mqtt_sensor_topic, "itho/sensor", sizeof(mqtt_sensor_topic));
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
  itho_sendjoin = 0;
  itho_forcemedium = 0;
  itho_vremapi = 0;
  itho_vremswap = 0;
  itho_rf_support = 0;
  rfInitOK = false;
  nonQ_cmd_clearsQ = 1;

  autopilot_active = 0;
  autopilot_updated = false;
  autopilot_hum_upper = 70; // default MUST be > than autopilot_hum_lower otherwise will cause config loading bootloop!
  autopilot_hum_lower = 40;
  autopilot_delay_upper = 0; // in seconds
  autopilot_delay_lower = 0; // in seconds

  itho_updated = false;
  get_itho_settings = false;
  get_autopilot_settings = false;
  configLoaded = false;

} //SystemConfig

// default destructor
SystemConfig::~SystemConfig() {
} //~SystemConfig

// return 0 on succes or the error code
uint8_t SystemConfig::set(JsonObjectConst obj) {
  uint8_t error = 1; // generic error

  if (!configLoaded) {
    if (obj["version_of_program"] != CONFIG_VERSION) {
      return 2; // config version error
    }
  }
  if (!(const char*)obj["sys_username"].isNull()) {
    error = 0;
    strlcpy(sys_username, obj["sys_username"], sizeof(sys_username));
  }
  if (!(const char*)obj["sys_password"].isNull()) {
    error = 0;
    strlcpy(sys_password, obj["sys_password"], sizeof(sys_password));
  }
  if (!(const char*)obj["syssec_web"].isNull()) {
    error = 0;
    syssec_web = obj["syssec_web"];
  }  
  if (!(const char*)obj["syssec_api"].isNull()) {
    error = 0;
    syssec_api = obj["syssec_api"];
  }
  if (!(const char*)obj["syssec_edit"].isNull()) {
    error = 0;
    syssec_edit = obj["syssec_edit"];
  }
  if (!(const char*)obj["syssht30"].isNull()) {
    error = 0;
    syssht30 = obj["syssht30"];
  }
  //MQTT Settings parse
  if (!(const char*)obj["mqtt_active"].isNull()) {
    mqtt_updated = true;
    error = 0;
    mqtt_active = obj["mqtt_active"];
  }
  if (!(const char*)obj["mqtt_serverName"].isNull()) {
    error = 0;
    strlcpy(mqtt_serverName, obj["mqtt_serverName"], sizeof(mqtt_serverName));
  }
  if (!(const char*)obj["mqtt_username"].isNull()) {
    error = 0;
    strlcpy(mqtt_username, obj["mqtt_username"], sizeof(mqtt_username));
  }
  if (!(const char*)obj["mqtt_password"].isNull()) {
    error = 0;
    strlcpy(mqtt_password, obj["mqtt_password"], sizeof(mqtt_password));
  }
  if (!(const char*)obj["mqtt_port"].isNull()) {
    error = 0;
    mqtt_port = obj["mqtt_port"];
  }
  if (!(const char*)obj["mqtt_version"].isNull()) {
    error = 0;
    mqtt_version = obj["mqtt_version"];
  }
  if (!(const char*)obj["mqtt_state_topic"].isNull()) {
    error = 0;
    strlcpy(mqtt_state_topic, obj["mqtt_state_topic"], sizeof(mqtt_state_topic));
  }
  if (!(const char*)obj["mqtt_sensor_topic"].isNull()) {
    error = 0;
    strlcpy(mqtt_sensor_topic, obj["mqtt_sensor_topic"], sizeof(mqtt_sensor_topic));
  }
  if (!(const char *)obj["mqtt_ha_topic"].isNull()) {
    error = 0;
    mqtt_updated = true;
    strlcpy(mqtt_ha_topic, obj["mqtt_ha_topic"], sizeof(mqtt_ha_topic));
  }
  if (!(const char*)obj["mqtt_state_retain"].isNull()) {
    error = 0;
    strlcpy(mqtt_state_retain, obj["mqtt_state_retain"], sizeof(mqtt_state_retain));
  }
  if (!(const char*)obj["mqtt_cmd_topic"].isNull()) {
    error = 0;
    strlcpy(mqtt_cmd_topic, obj["mqtt_cmd_topic"], sizeof(mqtt_cmd_topic));
  }
  if (!(const char*)obj["mqtt_lwt_topic"].isNull()) {
    error = 0;
    strlcpy(mqtt_lwt_topic, obj["mqtt_lwt_topic"], sizeof(mqtt_lwt_topic));
  }
  if (!(const char*)obj["mqtt_domoticz_active"].isNull()) {
    error = 0;
    mqtt_domoticz_active = obj["mqtt_domoticz_active"];
  }
  if (!(const char*)obj["mqtt_ha_active"].isNull()) {
    error = 0;
    mqtt_updated = true;
    mqtt_ha_active = obj["mqtt_ha_active"];
  }
  if (!(const char*)obj["mqtt_idx"].isNull()) {
    error = 0;
    mqtt_idx = obj["mqtt_idx"];
  }
  if (!(const char*)obj["sensor_idx"].isNull()) {
    error = 0;
    sensor_idx = obj["sensor_idx"];
  }
  if (!(const char*)obj["autopilot_active"].isNull()) {
    autopilot_updated = true;
    error = 0;
    autopilot_active = obj["autopilot_active"];
  }
  if (!(const char *)obj["autopilot_hum_upper"].isNull())
  {
    if (obj["autopilot_hum_upper"] > obj["autopilot_hum_lower"])
    {
      autopilot_updated = true;
      error = 0;
      autopilot_hum_upper = obj["autopilot_hum_upper"];
    }
    else
    {
      //TODO
      /* need to throw something like an user error msg*/
      return 20; // upper not > lower
    }
    
  }
  if (!(const char *)obj["autopilot_hum_lower"].isNull())
  {
    if (obj["autopilot_hum_upper"] > obj["autopilot_hum_lower"])
    {
      autopilot_updated = true;
      error = 0;
      autopilot_hum_lower = obj["autopilot_hum_lower"];
    }
    else
    {
      //TODO
      /* need to throw something like an user error msg */
      return 20; // upper not > lower
    }
    
  }
  if (!(const char *)obj["autopilot_delay_upper"].isNull()) {
    autopilot_updated = true;
    error = 0;
    autopilot_delay_upper = obj["autopilot_delay_upper"];
  }
  if (!(const char *)obj["autopilot_delay_lower"].isNull()) {
    autopilot_updated = true;
    error = 0;
    autopilot_delay_lower = obj["autopilot_delay_lower"];    
  }
  if (!(const char*)obj["itho_fallback"].isNull()) {
    //itho_updated = true;
    error = 0;
    itho_fallback = obj["itho_fallback"];
  }  
  if (!(const char*)obj["itho_low"].isNull()) {
    //itho_updated = true;
    error = 0;
    itho_low = obj["itho_low"];
  }
  if (!(const char*)obj["itho_medium"].isNull()) {
    error = 0;
    itho_medium = obj["itho_medium"];
  }
  if (!(const char*)obj["itho_high"].isNull()) {
    error = 0;
    itho_high = obj["itho_high"];
  }
  if (!(const char*)obj["itho_timer1"].isNull()) {
    error = 0;
    itho_timer1 = obj["itho_timer1"];
  }
  if (!(const char*)obj["itho_timer2"].isNull()) {
    error = 0;
    itho_timer2 = obj["itho_timer2"];
  }
  if (!(const char*)obj["itho_timer3"].isNull()) {
    error = 0;
    itho_timer3 = obj["itho_timer3"];
  }
  if (!(const char*)obj["itho_sendjoin"].isNull()) {
    error = 0;
    itho_sendjoin = obj["itho_sendjoin"];
  }
  if (!(const char*)obj["itho_forcemedium"].isNull()) {
    error = 0;
    itho_forcemedium = obj["itho_forcemedium"];
  }
  if (!(const char*)obj["itho_vremapi"].isNull()) {
    error = 0;
    itho_vremapi = obj["itho_vremapi"];
  }
  if (!(const char*)obj["itho_vremswap"].isNull()) {
    error = 0;
    itho_vremswap = obj["itho_vremswap"];
  }
  if (!(const char*)obj["itho_rf_support"].isNull()) {
    error = 0;
    itho_rf_support = obj["itho_rf_support"];
  }
  if (!(const char*)obj["rfInitOK"].isNull()) {
    error = 0;
    rfInitOK = obj["rfInitOK"];
  }
  if (!(const char*)obj["nonQ_cmd_clearsQ"].isNull()) {
    error = 0;
    nonQ_cmd_clearsQ = obj["nonQ_cmd_clearsQ"];
  }    
  return error;
}

void SystemConfig::get(JsonObject obj) const {

  bool complete = true;
  if (get_mqtt_settings || get_sys_settings || get_itho_settings || get_autopilot_settings) {
    complete = false;
  }
  if (complete || get_sys_settings) {
    get_sys_settings = false;
    obj["sys_username"] = sys_username;
    obj["sys_password"] = sys_password;
    obj["syssec_web"] = syssec_web;
    obj["syssec_api"] = syssec_api;
    obj["syssec_edit"] = syssec_edit;
    obj["syssht30"] = syssht30;
    obj["itho_rf_support"] = itho_rf_support;
    obj["rfInitOK"] = rfInitOK;    
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
    obj["mqtt_sensor_topic"] = mqtt_sensor_topic;
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
    obj["itho_fallback"] = itho_fallback;
    obj["itho_low"] = itho_low;
    obj["itho_medium"] = itho_medium;
    obj["itho_high"] = itho_high;
    obj["itho_timer1"] = itho_timer1;
    obj["itho_timer2"] = itho_timer2;
    obj["itho_timer3"] = itho_timer3;
    obj["itho_sendjoin"] = itho_sendjoin;
    obj["itho_forcemedium"] = itho_forcemedium;
    obj["itho_vremapi"] = itho_vremapi;
    obj["itho_vremswap"] = itho_vremswap;  
    obj["nonQ_cmd_clearsQ"] = nonQ_cmd_clearsQ;
  }
  if (complete || get_autopilot_settings) {
    get_autopilot_settings = false;
    obj["autopilot_active"] = autopilot_active;
    obj["autopilot_hum_upper"] = autopilot_hum_upper;
    obj["autopilot_hum_lower"] = autopilot_hum_lower;
    obj["autopilot_delay_upper"] = autopilot_delay_upper;
    obj["autopilot_delay_lower"] = autopilot_delay_lower;
  }
  obj["version_of_program"] = config_struct_version;
}
