

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

  DynamicJsonDocument root(1024);
  DeserializationError error = deserializeJson(root, configFile);
  if (error) {
    //Serial.println("wifi config load: DeserializationError");
    return false;
  }
    


  if (root["ssid"] == "") {
    //Serial.println("initial config still there, start WifiAP");
    return false;
  }

  if (wifiConfig.set(root.as<JsonObject>())) {

    //Serial.println("wifiConfig.set ok");

  }

  return true;
}

bool saveWifiConfig() {
  DynamicJsonDocument doc(2048);
  JsonObject root = doc.to<JsonObject>(); // Fill the object
  wifiConfig.get(root);
  
  File configFile = SPIFFS.open("/wifi.json", "w");
  if (!configFile) {
    //Serial.println("Failed to open default config file for writing");
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

  DynamicJsonDocument root(2048);
  DeserializationError error = deserializeJson(root, configFile);
  if (error)
    return false;

  systemConfig.configLoaded = systemConfig.set(root.as<JsonObject>());
  //Serial.print("config :"); Serial.println(result);
  
  if (!systemConfig.configLoaded) {
    logInput("Config version mismatch, resetting config...");
    saveSystemConfig();
    delay(1000);
    ESP.restart();
  }
  
  return true;
}

bool saveSystemConfig() {
  DynamicJsonDocument doc(2048);
  JsonObject root = doc.to<JsonObject>(); // Fill the object
  systemConfig.get(root);
  
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
