

bool loadWifiConfig() {
  if (!SPIFFS.exists("/wifi.json")) {
    //Serial.println("Writing initial wifi config");
    if (!saveWifiConfig()) {
      //Serial.println("Failed writing initial default config");
      return false;
    }
  }

  File configFile = SPIFFS.open("/wifi.json", "r");
  if (!configFile) {
    //Serial.println("Failed to open wifi config file");
    return false;
  }
  //SPIFFS.exists(path)

  size_t size = configFile.size();
  if (size > 1024) {
    //Serial.println("Wifi config file size is too large");
    return false;
  }

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable. If you don't use ArduinoJson, you may as well
  // use configFile.readString instead.
  configFile.readBytes(buf.get(), size);

  DynamicJsonDocument root(1024);
  DeserializationError error = deserializeJson(root, buf.get());
  if (error)
    return false;


  if (root["ssid"] == "") {
    //initial config still there, start WifiAP
    return false;
  }

  strlcpy(wifiConfig.ssid, root[F("ssid")], sizeof(wifiConfig.ssid));
  strlcpy(wifiConfig.passwd, root[F("passwd")], sizeof(wifiConfig.passwd));
  strlcpy(wifiConfig.dhcp, root[F("dhcp")], sizeof(wifiConfig.dhcp));
  wifiConfig.renew = root[F("renew")];
  strlcpy(wifiConfig.ip, root[F("ip")], sizeof(wifiConfig.ip));
  strlcpy(wifiConfig.subnet, root[F("subnet")], sizeof(wifiConfig.subnet));
  strlcpy(wifiConfig.gateway, root[F("gateway")], sizeof(wifiConfig.gateway));
  strlcpy(wifiConfig.dns1, root[F("dns1")], sizeof(wifiConfig.dns1));
  strlcpy(wifiConfig.dns2, root[F("dns2")], sizeof(wifiConfig.dns2));
  wifiConfig.port = root[F("port")];

  return true;
}

bool saveWifiConfig() {
  DynamicJsonDocument root(1024);
  //JsonObject root = jsonWifiConf.createObject();
  root[F("ssid")] = wifiConfig.ssid;
  root[F("passwd")] = wifiConfig.passwd;
  root[F("dhcp")] = wifiConfig.dhcp;
  root[F("renew")] = wifiConfig.renew;
  root[F("ip")] = wifiConfig.ip;
  root[F("subnet")] = wifiConfig.subnet;
  root[F("gateway")] = wifiConfig.gateway;
  root[F("dns1")] = wifiConfig.dns1;
  root[F("dns2")] = wifiConfig.dns2;
  root[F("port")] = wifiConfig.port;

  File configFile = SPIFFS.open("/wifi.json", "w");
  if (!configFile) {
    //Serial.println("Failed to open default wifi config file for writing");
    return false;
  }

  serializeJson(root, configFile);

  return true;
}

bool resetWifiConfig() {
  if (!SPIFFS.remove("/wifi.json")) {
    return false;
  }
  if (!SPIFFS.exists("/wifi.json")) {
    return true;
  }
}

bool loadSystemConfig() {

  if (!SPIFFS.exists("/config.json")) {
    //Serial.println("Writing initial default config");
    if (!saveSystemConfig()) {
      //Serial.println("Failed writing initial default config");
      return false;
    }
  }
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    //Serial.println("Failed to open system config file");
    return false;
  }

  size_t size = configFile.size();
  if (size > 2048) {
    //Serial.println("System config file size is too large");
    return false;
  }

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable. If you don't use ArduinoJson, you may as well
  // use configFile.readString instead.
  configFile.readBytes(buf.get(), size);

  DynamicJsonDocument root(2048);
  DeserializationError error = deserializeJson(root, buf.get());
  if (error)
    return false;

  if (root[F("version_of_program")] != CONFIG_VERSION) {
    logInput("Config version mismatch, resetting config...");
    saveSystemConfig();
    delay(1000);
    ESP.restart();
  }
  
  strlcpy(systemConfig.mqtt_active, root[F("mqtt_active")], sizeof(systemConfig.mqtt_active));
  strlcpy(systemConfig.mqtt_serverName, root[F("mqtt_serverName")], sizeof(systemConfig.mqtt_serverName));
  strlcpy(systemConfig.mqtt_username, root[F("mqtt_username")], sizeof(systemConfig.mqtt_username));
  strlcpy(systemConfig.mqtt_password, root[F("mqtt_password")], sizeof(systemConfig.mqtt_password));
  systemConfig.mqtt_port = root[F("mqtt_port")];
  systemConfig.mqtt_version = root[F("mqtt_version")];
  strlcpy(systemConfig.mqtt_state_topic, root[F("mqtt_state_topic")], sizeof(systemConfig.mqtt_state_topic));
  strlcpy(systemConfig.mqtt_state_retain, root[F("mqtt_state_retain")], sizeof(systemConfig.mqtt_state_retain));
  strlcpy(systemConfig.mqtt_cmd_topic, root[F("mqtt_cmd_topic")], sizeof(systemConfig.mqtt_cmd_topic));
  strlcpy(systemConfig.mqtt_domoticz_active, root[F("mqtt_domoticz_active")], sizeof(systemConfig.mqtt_domoticz_active));
  systemConfig.mqtt_idx = root[F("mqtt_idx")];
  strlcpy(systemConfig.version_of_program, root[F("version_of_program")], sizeof(systemConfig.version_of_program));
  //Serial.println("System config loaded");

  return true;
}

bool saveSystemConfig() {
  DynamicJsonDocument root(2048);
  //JsonObject root = jsonSystemConf.createObject();
  root["mqtt_active"] = systemConfig.mqtt_active;
  root["mqtt_serverName"] = systemConfig.mqtt_serverName;
  root["mqtt_username"] = systemConfig.mqtt_username;
  root["mqtt_password"] = systemConfig.mqtt_password;
  root["mqtt_port"] = systemConfig.mqtt_port;
  root["mqtt_version"] = systemConfig.mqtt_version;
  root["mqtt_state_topic"] = systemConfig.mqtt_state_topic;
  root["mqtt_state_retain"] = systemConfig.mqtt_state_retain;
  root["mqtt_cmd_topic"] = systemConfig.mqtt_cmd_topic;
  root["mqtt_domoticz_active"] = systemConfig.mqtt_domoticz_active;
  root["mqtt_idx"] = systemConfig.mqtt_idx;    
  root["version_of_program"] = systemConfig.version_of_program;

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    //Serial.println("Failed to open default config file for writing");
    return false;
  }

  serializeJson(root, configFile);

  return true;
}

bool resetSystemConfig() {
  if (!SPIFFS.remove("/config.json")) {
    return false;
  }
  if (!SPIFFS.exists("/config.json")) {
    return true;
  }
}
