

bool loadWifiConfig() {
  if (!SPIFFS.exists("/wifi.json")) {
    D_LOG("Setup: writing initial wifi config\n");
    if (!saveWifiConfig()) {
      D_LOG("Setup: failed writing initial wifi config\n");
      return false;
    }
  }

  File configFile = SPIFFS.open("/wifi.json", "r");
  if (!configFile) {
    D_LOG("Setup: failed to open wifi config file\n");
    return false;
  }
  //SPIFFS.exists(path)

  size_t size = configFile.size();
  if (size > 1024) {
    D_LOG("Setup: wifi config file size is too large\n");
    return false;
  }

  DynamicJsonDocument root(1024);
  DeserializationError error = deserializeJson(root, configFile);
  if (error) {
    D_LOG("Setup: wifi config load DeserializationError\n");
    return false;
  }

  if (root["ssid"] == "") {
    D_LOG("Setup: initial wifi config still there, start WifiAP\n");
    return false;
  }
  if (wifiConfig.set(root.as<JsonObject>())) {
  }
  D_LOG("Setup: wifi config loaded successful\n");
  return true;
}

bool saveWifiConfig() {
  DynamicJsonDocument doc(2048);
  JsonObject root = doc.to<JsonObject>();
  // Fill the object
  wifiConfig.get(root);

  File configFile = SPIFFS.open("/wifi.json", "w");
  if (!configFile) {
    D_LOG("Failed to open default config file for writing\n");
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
  return false;
}

bool loadSystemConfig() {

  if (!SPIFFS.exists("/config.json")) {
    D_LOG("Writing initial default config\n");
    if (!saveSystemConfig()) {
      D_LOG("Failed writing initial default config\n");
      return false;
    }
  }
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    D_LOG("Failed to open system config file\n");
    return false;
  }

  size_t size = configFile.size();
  if (size > 2048) {
    D_LOG("System config file size is too large\n");
    return false;
  }

  DynamicJsonDocument root(2048);
  DeserializationError error = deserializeJson(root, configFile);
  if (error)
    return false;

  systemConfig.configLoaded = systemConfig.set(root.as<JsonObject>());
  D_LOG("config : %d\n", systemConfig.configLoaded);

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
  JsonObject root = doc.to<JsonObject>(); 

  systemConfig.get(root);

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    D_LOG("Failed to open default config file for writing\n");
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
  return false;
}
#if defined (HW_VERSION_TWO)

uint16_t serializeRemotes(const IthoRemote &remotes, Print& dst) {
  DynamicJsonDocument doc(1000 + (MAX_NUMBER_OF_REMOTES * 300));

  JsonObject root = doc.to<JsonObject>(); 

  remotes.get(root);

  return serializeJson(doc, dst) > 0;
}


bool saveRemotesConfig() {

  return saveFileRemotes("/remotes.json", remotes);

}

bool saveFileRemotes(const char *filename, const IthoRemote &remotes) { // Open file for writing
  File file = SPIFFS.open(filename, "w");
  if (!file) {
    D_LOG("Failed to create remotes file\n");
    return false;
  }
  // Serialize JSON to file
  bool success = serializeRemotes(remotes, file);
  if (!success) {
    D_LOG("Failed to serialize remotes\n");
    return false;
  }
  D_LOG("File saved\n");
  return true;
}

bool deserializeRemotes(Stream &src, IthoRemote &remotes) {

  DynamicJsonDocument doc(1000 + (MAX_NUMBER_OF_REMOTES * 300));

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

remotes.configLoaded = remotes.set(doc.as<JsonObject>());
  if (!remotes.configLoaded) {
    logInput("Remote config version mismatch, resetting config...");
    saveRemotesConfig();
    delay(0);
    if (!loadRemotesConfig()) {
      logInput("Remote config load failed, reboot...");
      delay(1000);
      ESP.restart();
    }

  }


  return true;
}

bool loadRemotesConfig() {

  return loadFileRemotes("/remotes.json", remotes);

}

bool loadFileRemotes(const char *filename, IthoRemote &remotes) { // Open file for reading
  File file = SPIFFS.open(filename, "r");

  if (!file) {
    D_LOG("Failed to open config file\n"); 
    return false;
  }

  bool success = deserializeRemotes(file, remotes);

  if (!success) {
    D_LOG("Failed to deserialize configuration\n");
    return false;
  }
  return true;
}
#endif
