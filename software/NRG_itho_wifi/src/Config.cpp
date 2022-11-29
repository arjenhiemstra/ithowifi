#include "Config.h"

bool loadWifiConfig()
{
  if (!ACTIVE_FS.exists("/wifi.json"))
  {
    D_LOG("Setup: writing initial wifi config");
    if (!saveWifiConfig())
    {
      E_LOG("Setup: failed writing initial wifi config");
      return false;
    }
  }

  File configFile = ACTIVE_FS.open("/wifi.json", "r");
  if (!configFile)
  {
    D_LOG("Setup: failed to open wifi config file");
    return false;
  }
  // ACTIVE_FS.exists(path)

  size_t size = configFile.size();
  if (size > 1024)
  {
    E_LOG("Setup: wifi config file size is too large");
    return false;
  }

  DynamicJsonDocument root(1024);
  DeserializationError error = deserializeJson(root, configFile);
  if (error)
  {
    E_LOG("Setup: wifi config load DeserializationError");
    return false;
  }

  if (root["ssid"] == "")
  {
    I_LOG("Setup: initial wifi config still there, start WifiAP");
    return false;
  }
  if (wifiConfig.set(root.as<JsonObject>()))
  {
  }
  D_LOG("Setup: wifi config loaded successful");
  return true;
}

bool saveWifiConfig()
{
  DynamicJsonDocument doc(2048);
  JsonObject root = doc.to<JsonObject>();
  // Fill the object
  wifiConfig.get(root);

  File configFile = ACTIVE_FS.open("/wifi.json", "w");
  if (!configFile)
  {
    E_LOG("Failed to open default config file for writing");
    return false;
  }

  serializeJson(root, configFile);

  I_LOG("wifi config saved");
  return true;
}

bool resetWifiConfig()
{
  if (!ACTIVE_FS.remove("/wifi.json"))
  {
    return false;
  }
  if (!ACTIVE_FS.exists("/wifi.json"))
  {
    dontSaveConfig = true;
    return true;
  }
  return false;
}

bool loadSystemConfig()
{

  if (!ACTIVE_FS.exists("/config.json"))
  {
    I_LOG("Writing initial default config");
    if (!saveSystemConfig())
    {
      E_LOG("Failed writing initial default config");
      return false;
    }
  }
  File configFile = ACTIVE_FS.open("/config.json", "r");
  if (!configFile)
  {
    E_LOG("Failed to open system config file");
    return false;
  }

  size_t size = configFile.size();
  if (size > 2048)
  {
    E_LOG("System config file size is too large");
    return false;
  }

  DynamicJsonDocument root(2048);
  DeserializationError error = deserializeJson(root, configFile);
  if (error)
    return false;

  systemConfig.configLoaded = systemConfig.set(root.as<JsonObject>());
  D_LOG("config : %d", systemConfig.configLoaded);

  if (!systemConfig.configLoaded)
  {
    W_LOG("Config version mismatch, resetting config...");
    saveSystemConfig();
    delay(1000);
    ESP.restart();
  }

  ithoQueue.set_itho_fallback_speed(systemConfig.itho_fallback);

  return true;
}

bool saveSystemConfig()
{
  DynamicJsonDocument doc(2048);
  JsonObject root = doc.to<JsonObject>();

  systemConfig.get(root);

  File configFile = ACTIVE_FS.open("/config.json", "w");
  if (!configFile)
  {
    E_LOG("Failed to open default config file for writing");
    return false;
  }

  serializeJson(root, configFile);

  return true;
}

bool resetSystemConfig()
{
  if (!ACTIVE_FS.remove("/config.json"))
  {
    return false;
  }
  if (!ACTIVE_FS.exists("/config.json"))
  {
    dontSaveConfig = true;
    return true;
  }
  return false;
}

uint16_t serializeRemotes(const char *filename, const IthoRemote &remotes, Print &dst)
{
  DynamicJsonDocument doc(1000 + (MAX_NUMBER_OF_REMOTES * 400));

  JsonObject root = doc.to<JsonObject>();

  std::string tempStr = filename;
  if (tempStr.find(".json") == std::string::npos)
    return 0;

  std::string rootName = tempStr.substr(1, static_cast<char>(tempStr.find(".json")));
  remotes.get(root, rootName.c_str());

  return serializeJson(doc, dst) > 0;
}

bool saveRemotesConfig()
{

  return saveFileRemotes("/remotes.json", remotes);
}

bool saveVirtualRemotesConfig()
{

  return saveFileRemotes("/vremotes.json", virtualRemotes);
}

bool saveFileRemotes(const char *filename, const IthoRemote &remotes)
{ // Open file for writing
  File file = ACTIVE_FS.open(filename, "w");
  if (!file)
  {
    E_LOG("Failed to create remotes file");
    return false;
  }
  // Serialize JSON to file
  bool success = serializeRemotes(filename, remotes, file);
  if (!success)
  {
    E_LOG("Failed to serialize remotes");
    return false;
  }
  D_LOG("File saved");
  return true;
}

bool deserializeRemotes(const char *filename, Stream &src, IthoRemote &remotes)
{

  DynamicJsonDocument doc(1000 + (MAX_NUMBER_OF_REMOTES * 400));

  DeserializationError err = deserializeJson(doc, src);
  if (err)
  {
    E_LOG("Failed to deserialize remotes config json");
    return false;
  }

  doc.shrinkToFit();

  std::string tempStr = filename;
  if (tempStr.find(".json") == std::string::npos)
    return false;

  std::string rootName = tempStr.substr(1, static_cast<char>(tempStr.find(".json")));

  remotes.configLoaded = remotes.set(doc.as<JsonObject>(), rootName.c_str());
  if (!remotes.configLoaded)
  {
    W_LOG("Remotes config version mismatch, resetting config...");
    saveFileRemotes(filename, remotes);
    delay(1000);
    ESP.restart();
  }

  return true;
}

bool loadRemotesConfig()
{

  return loadFileRemotes("/remotes.json", remotes);
}

bool loadVirtualRemotesConfig()
{

  return loadFileRemotes("/vremotes.json", virtualRemotes);
}

bool loadFileRemotes(const char *filename, IthoRemote &remotes)
{ // Open file for reading

  if (!ACTIVE_FS.exists(filename))
  {
    N_LOG("Setup: writing initial remotes config");
    if (!saveFileRemotes(filename, remotes))
    {
      E_LOG("Setup: failed writing initial remotes config");
      return false;
    }
  }

  File file = ACTIVE_FS.open(filename, "r");

  if (!file)
  {
    E_LOG("Failed to open config file");
    return false;
  }

  bool success = deserializeRemotes(filename, file, remotes);

  if (!success)
  {
    E_LOG("Failed to deserialize configuration");
    return false;
  }
  I_LOG("Setup: remotes configfile loaded");
  return true;
}

bool loadLogConfig()
{
  if (!ACTIVE_FS.exists("/syslog.json"))
  {
    I_LOG("Setup: writing initial syslog config");
    if (!saveLogConfig())
    {
      E_LOG("Setup: failed writing initial syslog config");
      return false;
    }
  }

  File configFile = ACTIVE_FS.open("/syslog.json", "r");
  if (!configFile)
  {
    E_LOG("Setup: failed to open syslog config file");
    return false;
  }
  // ACTIVE_FS.exists(path)

  size_t size = configFile.size();
  if (size > 1024)
  {
    E_LOG("Setup: syslog config file size is too large");
    return false;
  }

  DynamicJsonDocument root(1024);
  DeserializationError error = deserializeJson(root, configFile);
  if (error)
  {
    E_LOG("Setup: syslog config load Deserialization Error");
    return false;
  }
  logConfig.configLoaded = logConfig.set(root.as<JsonObject>());

  I_LOG("Setup: syslog config loaded : %d", logConfig.configLoaded);

  return true;
}

bool saveLogConfig()
{
  DynamicJsonDocument doc(2048);
  JsonObject root = doc.to<JsonObject>();
  // Fill the object
  logConfig.get(root);

  File configFile = ACTIVE_FS.open("/syslog.json", "w");
  if (!configFile)
  {
    E_LOG("Failed to open default config file for writing");
    return false;
  }

  serializeJson(root, configFile);

  I_LOG("syslog config saved");
  return true;
}

bool resetLogConfig()
{
  if (!ACTIVE_FS.remove("/syslog.json"))
  {
    return false;
  }
  if (!ACTIVE_FS.exists("/syslog.json"))
  {
    dontSaveConfig = true;
    return true;
  }
  return false;
}
