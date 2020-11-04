#include "SystemConfig.h"
#include <string.h>
#include <Arduino.h>


// default constructor
SystemConfig::SystemConfig() {
  //default config
  strlcpy(config_struct_version, CONFIG_VERSION, sizeof(config_struct_version));
  strlcpy(mqtt_active, "off", sizeof(mqtt_active));
  strlcpy(mqtt_serverName, "192.168.1.123", sizeof(mqtt_serverName));
  strlcpy(mqtt_username, "", sizeof(mqtt_username));
  strlcpy(mqtt_password, "", sizeof(mqtt_password));
  mqtt_port = 1883;
  mqtt_version = 1;
  strlcpy(mqtt_state_topic, "itho/state", sizeof(mqtt_state_topic));
  strlcpy(mqtt_state_retain, "yes", sizeof(mqtt_state_retain));
  strlcpy(mqtt_cmd_topic, "itho/cmd", sizeof(mqtt_cmd_topic));
  strlcpy(mqtt_domoticz_active, "off", sizeof(mqtt_domoticz_active));

} //SystemConfig

// default destructor
SystemConfig::~SystemConfig() {
} //~SystemConfig


bool SystemConfig::set(JsonObjectConst obj) {
  bool updated = false;

  //MQTT Settings parse
  if (!(const char*)obj[F("mqtt_active")].isNull()) {
    updated = true;
    strlcpy(mqtt_active, obj[F("mqtt_active")], sizeof(mqtt_active));
  }
  if (!(const char*)obj[F("mqtt_serverName")].isNull()) {
    updated = true;
    strlcpy(mqtt_serverName, obj[F("mqtt_serverName")], sizeof(mqtt_serverName));
  }
  if (!(const char*)obj[F("mqtt_username")].isNull()) {
    updated = true;
    strlcpy(mqtt_username, obj[F("mqtt_username")], sizeof(mqtt_username));
  }
  if (!(const char*)obj[F("mqtt_password")].isNull()) {
    updated = true;
    strlcpy(mqtt_password, obj[F("mqtt_password")], sizeof(mqtt_password));
  }
  if (!(const char*)obj[F("mqtt_port")].isNull()) {
    updated = true;
    mqtt_port = obj[F("mqtt_port")];
  }
  if (!(const char*)obj[F("mqtt_version")].isNull()) {
    updated = true;
    mqtt_version = obj[F("mqtt_version")];
  }
  if (!(const char*)obj[F("mqtt_state_topic")].isNull()) {
    updated = true;
    strlcpy(mqtt_state_topic, obj[F("mqtt_state_topic")], sizeof(mqtt_state_topic));
  }
  if (!(const char*)obj[F("mqtt_state_retain")].isNull()) {
    updated = true;
    strlcpy(mqtt_state_retain, obj[F("mqtt_state_retain")], sizeof(mqtt_state_retain));
  }
  if (!(const char*)obj[F("mqtt_cmd_topic")].isNull()) {
    updated = true;
    strlcpy(mqtt_cmd_topic, obj[F("mqtt_cmd_topic")], sizeof(mqtt_cmd_topic));
  }
  if (!(const char*)obj[F("mqtt_domoticz_active")].isNull()) {
    updated = true;
    strlcpy(mqtt_domoticz_active, obj[F("mqtt_domoticz_active")], sizeof(mqtt_domoticz_active));
  }
  if (!(const char*)obj[F("mqtt_idx")].isNull()) {
    updated = true;
    mqtt_idx = obj[F("mqtt_idx")];
  }
  return updated;
}

void SystemConfig::get(JsonObject obj) const {
  obj["mqtt_active"] = mqtt_active;
  obj["mqtt_serverName"] = mqtt_serverName;
  obj["mqtt_username"] = mqtt_username;
  obj["mqtt_password"] = mqtt_password;
  obj["mqtt_port"] = mqtt_port;
  obj["mqtt_version"] = mqtt_version;
  obj["mqtt_state_topic"] = mqtt_state_topic;
  obj["mqtt_state_retain"] = mqtt_state_retain;
  obj["mqtt_cmd_topic"] = mqtt_cmd_topic;
  obj["mqtt_domoticz_active"] = mqtt_domoticz_active;
  obj["mqtt_idx"] = mqtt_idx;
  obj["version_of_program"] = config_struct_version;
}
