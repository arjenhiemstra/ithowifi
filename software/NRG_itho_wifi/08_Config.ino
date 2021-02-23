

bool loadWifiConfig() {
  if (!SPIFFS.exists("/wifi.json")) {
    #if defined (INFORMATIVE_LOGGING)
    logInput("Setup: writing initial wifi config");
    #endif
    //Serial.println("Writing initial wifi config");
    if (!saveWifiConfig()) {
      #if defined (INFORMATIVE_LOGGING)
      logInput("Setup: failed writing initial wifi config");
      #endif
      //Serial.println("Failed writing initial default config");
      return false;
    }
  }

  File configFile = SPIFFS.open("/wifi.json", "r");
  if (!configFile) {
    #if defined (INFORMATIVE_LOGGING)
    logInput("Setup: failed to open wifi config file");
    #endif
    //Serial.println("Failed to open wifi config file");
    return false;
  }
  //SPIFFS.exists(path)

  size_t size = configFile.size();
  if (size > 1024) {
    #if defined (INFORMATIVE_LOGGING)
    logInput("Setup: wifi config file size is too large");
    #endif
    //Serial.println("Wifi config file size is too large");
    return false;
  }

  DynamicJsonDocument root(1024);
  DeserializationError error = deserializeJson(root, configFile);
  if (error) {
    #if defined (INFORMATIVE_LOGGING)
    logInput("Setup: wifi config load DeserializationError");
    #endif
    //Serial.println("wifi config load: DeserializationError");
    return false;
  }



  if (root["ssid"] == "") {
    #if defined (INFORMATIVE_LOGGING)
    logInput("Setup: initial wifi config still there, start WifiAP");
    #endif
    //Serial.println("initial config still there, start WifiAP");
    return false;
  }

  if (wifiConfig.set(root.as<JsonObject>())) {

    //Serial.println("wifiConfig.set ok");

  }
  #if defined (INFORMATIVE_LOGGING)
  logInput("Setup: wifi config loaded successful");
  #endif
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

  logInput("wifi config saved");
  return true;

}

bool resetWifiConfig() {
  if (!SPIFFS.remove("/wifi.json")) {
    return false;
  }
  if (!SPIFFS.exists("/wifi.json")) {
    dontSaveConfig = true;
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

  ithoQueue.set_itho_fallback_speed(systemConfig.itho_fallback);

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
    dontSaveConfig = true;
    return true;
  }
}
#if defined (__HW_VERSION_TWO__)

uint16_t serializeRemotes(const IthoRemote &remotes, Print& dst) {
  DynamicJsonDocument doc(1000 + (MAX_NUMBER_OF_REMOTES * 300));
  // Create an object at the root
  JsonObject root = doc.to<JsonObject>(); // Fill the object
  remotes.get(root);
  // Serialize JSON to file
  return serializeJson(doc, dst) > 0;
}


bool saveRemotesConfig() {

  return saveFileRemotes("/remotes.json", remotes);

}

bool saveFileRemotes(const char *filename, const IthoRemote &remotes) { // Open file for writing
  File file = SPIFFS.open(filename, "w");
  if (!file) {
    //Serial.println(F("Failed to create remotes file"));
    return false;
  }
  // Serialize JSON to file
  bool success = serializeRemotes(remotes, file);
  if (!success) {
    //Serial.println(F("Failed to serialize remotes"));
    return false;
  }
  //Serial.println(F("File saved"));
  return true;
}

bool deserializeRemotes(Stream &src, IthoRemote &remotes) {

  DynamicJsonDocument doc(1000 + (MAX_NUMBER_OF_REMOTES * 300));

  // Parse the JSON object in the file
  DeserializationError err = deserializeJson(doc, src);
  if (err) {
    logInput("Failed to deserialize remotes config json");
    saveRemotesConfig();
    if (!loadRemotesConfig()) {
      logInput("Remote config load failed, reboot...");
      delay(1000);
      ESP.restart();
    }
    return false;
  }

  doc.shrinkToFit();

  remotes.set(doc.as<JsonObject>());

//remotes.configLoaded = remotes.set(doc.as<JsonObject>());
//  if (!remotes.configLoaded) {
//    logInput("Remote config version mismatch, resetting config...");
//    saveRemotesConfig();
//    delay(0);
//    if (!loadRemotesConfig()) {
//      logInput("Remote config load failed, reboot...");
//      delay(1000);
//      ESP.restart();
//    }
//
//  }


  return true;
}

bool loadRemotesConfig() {

  return loadFileRemotes("/remotes.json", remotes);

}

bool loadFileRemotes(const char *filename, IthoRemote &remotes) { // Open file for reading
  File file = SPIFFS.open(filename, "r");
  // This may fail if the file is missing
  if (!file) {
    //Serial.println(F("Failed to open config file")); return false;
  }
  // Parse the JSON object in the file
  bool success = deserializeRemotes(file, remotes);
  // This may fail if the JSON is invalid
  if (!success) {
    //Serial.println(F("Failed to deserialize configuration"));
    return false;
  }
  return true;
}
#endif
