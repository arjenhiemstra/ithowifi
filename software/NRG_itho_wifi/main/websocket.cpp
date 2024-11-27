#include "websocket.h"

#if defined MG_ENABLE_PACKED_FS && MG_ENABLE_PACKED_FS == 1
#else
AsyncWebServer wsserver(8000);
#endif

#if defined MG_ENABLE_PACKED_FS && MG_ENABLE_PACKED_FS == 1
static void wsEvent(struct mg_connection *c, int ev, void *ev_data);
#else
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
#endif

void websocketInit()
{
#if defined MG_ENABLE_PACKED_FS && MG_ENABLE_PACKED_FS == 1
  mg_log_set(0);
  mg_mgr_init(&mgr);                                   // Initialise event manager
  mg_http_listen(&mgr, s_listen_on_ws, wsEvent, &mgr); // Create WS listener
#else
  // attach AsyncWebSocket
  ws.onEvent(onWsEvent);
  wsserver.addHandler(&ws);
  wsserver.begin();
  WebSerial.onMessage([](const std::string &msg)
                      { Serial.println(msg.c_str()); });

  if (systemConfig.syssec_web)
  {
    WebSerial.setAuthentication(systemConfig.sys_username, systemConfig.sys_password);
  }
  // if enable a web based serial terminal will be available at http://IP:8000/webserial
  if (logConfig.webserial_active)
  {
    WebSerial.begin(&wsserver);
  }

#endif
}

void jsonWsSend(const char *rootName)
{
  JsonDocument root;

  if (strcmp(rootName, "wifisettings") == 0)
  {
    JsonObject nested = root[rootName].to<JsonObject>();
    wifiConfig.get(nested);
    if (strcmp(wifiConfig.dhcp, "on") == 0)
    {
      nested["ip"] = WiFi.localIP().toString();
      nested["subnet"] = WiFi.subnetMask().toString();
      nested["gateway"] = WiFi.gatewayIP().toString();
      nested["dns1"] = WiFi.dnsIP().toString();
      nested["dns2"] = WiFi.dnsIP(1).toString();
    }
    if (strcmp(wifiConfig.hostname, "") == 0) // defualt config still there
    {
      nested["hostname"] = hostName();
    }
  }
  else if (strcmp(rootName, "wifistat") == 0)
  {

    JsonObject wifiinfo = root[rootName].to<JsonObject>();
    IPAddress ip = WiFi.localIP();
    char wifiip[16] = {};
    snprintf(wifiip, sizeof(wifiip), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

    char apremain[16] = {};
    long timeout = wifiConfig.aptimeout * 60 * 1000;
    long time = (millis() - APmodeTimeout) < timeout ? (timeout - (millis() - APmodeTimeout)) / 1000 : 0;

    if (!wifiModeAP || wifiConfig.aptimeout == 0)
    {
      snprintf(apremain, sizeof(apremain), "%s", "off");
    }
    else if (wifiConfig.aptimeout == 255)
    {
      snprintf(apremain, sizeof(apremain), "%s", "always on");
    }
    else
    {
      snprintf(apremain, sizeof(apremain), "%ld sec.", time);
    }

    static char apssid[32]{};

    snprintf(apssid, sizeof(apssid), "%s%02x%02x", espName, sys.getMac(4), sys.getMac(5));

    wifiinfo["wifissid"] = WiFi.SSID();
    wifiinfo["wifimac"] = WiFi.macAddress();
    wifiinfo["wificonnstat"] = wifiConfig.wl_status_to_name(WiFi.status());
    wifiinfo["wifiip"] = wifiip;
    wifiinfo["apactive"] = wifiModeAP ? "yes" : "no";
    wifiinfo["apremain"] = apremain;
    wifiinfo["apssid"] = apssid;
    wifiinfo["apip"] = "192.168.4.1";
  }
  else if (strcmp(rootName, "systemsettings") == 0)
  {
    JsonObject nested = root[rootName].to<JsonObject>();
    systemConfig.get(nested);
  }
  else if (strcmp(rootName, "logsettings") == 0)
  {
    JsonObject nested = root[rootName].to<JsonObject>();
    logConfig.get(nested);
    if (prevlog_available())
    {
      nested["prevlog"] = "/prevlog";
    }
  }
  else if (strcmp(rootName, "ithodevinfo") == 0)
  {
    JsonObject nested = root[rootName].to<JsonObject>();
    nested["itho_devtype"] = getIthoType();
    nested["itho_mfr"] = currentIthoDeviceGroup();
    nested["itho_hwversion"] = currentItho_hwversion();
    nested["itho_fwversion"] = currentItho_fwversion();
    nested["itho_setlen"] = currentIthoSettingsLength();
  }
  else if (strcmp(rootName, "ithostatusinfo") == 0)
  {
    JsonObject nested = root[rootName].to<JsonObject>();
    uint8_t count = getIthoStatusJSON(nested);
    root["count"] = count;
  }
  else if (strcmp(rootName, "debuginfo") == 0)
  {
    JsonObject nested = root[rootName].to<JsonObject>();
    nested["configversion"] = CONFIG_VERSION;
    nested["bfree"] = ACTIVE_FS.usedBytes();
    nested["btotal"] = ACTIVE_FS.totalBytes();
    nested["bfree"] = ACTIVE_FS.usedBytes();
    nested["cc1101taskmem"] = TaskCC1101HWmark;
    nested["mqtttaskmem"] = TaskMQTTHWmark;
    nested["webtaskmem"] = TaskWebHWmark;
    nested["cltaskmem"] = TaskConfigAndLogHWmark;
    nested["syscontaskmem"] = TaskSysControlHWmark;
    TaskHandle_t loopTaskHandle = xTaskGetHandle("loopTask");
    if (loopTaskHandle != NULL)
    {
      nested["looptaskmem"] = static_cast<uint32_t>(uxTaskGetStackHighWaterMark(loopTaskHandle));
    }
  }
  else if (strcmp(rootName, "i2cdebuglog") == 0) // i2cdebuglog
  {
    i2cLogger.get(root.to<JsonObject>(), rootName);
  }
  else if (strcmp(rootName, "remtypeconf") == 0)
  {
    JsonObject nested = root[rootName].to<JsonObject>();
    nested["remtype"] = static_cast<uint16_t>(virtualRemotes.getRemoteType(0)); // FIXME, should also support remotes on other positions than only index 0
  }
  else if (strcmp(rootName, "remotes") == 0)
  {
    JsonObject obj = root.to<JsonObject>(); // Fill the object
    remotes.get(obj, rootName);
  }
  else if (strcmp(rootName, "vremotes") == 0)
  {
    JsonObject obj = root.to<JsonObject>(); // Fill the object
    virtualRemotes.get(obj, rootName);
  }
  else if (strcmp(rootName, "chkpart") == 0)
  {
    root["chkpart"] = chk_partition_res ? "partition scheme supports firmware versions from 2.4.4-beta7 and upwards" : "partion scheme supports firmware versions 2.4.4-beta6 and older";
  }
  notifyClients(root.as<JsonObject>());
}

void handle_ws_message(std::string &&msg)
{

  D_LOG("%s", msg.c_str());

  if (msg.find("{\"wifiscan\"") != std::string::npos)
  {
    D_LOG("Start wifi scan");
    runscan = true;
  }
  if (msg.find("{\"sysstat\"") != std::string::npos)
  {
    sysStatReq = true;
  }
  if (msg.find("{\"remtype\"") != std::string::npos)
  {
    jsonWsSend("remtypeconf");
  }
  if (msg.find("{\"debugvalues\"") != std::string::npos)
  {
    jsonWsSend("debuginfo");
  }
  else if (msg.find("{\"i2cdebuglog\"") != std::string::npos)
  {
    jsonWsSend("i2cdebuglog");
  }
  else if (msg.find("{\"wifisettings\"") != std::string::npos || msg.find("{\"systemsettings\"") != std::string::npos || msg.find("{\"logsettings\"") != std::string::npos)
  {
    JsonDocument root;
    DeserializationError error = deserializeJson(root, msg.c_str());
    if (!error)
    {
      JsonObject obj = root.as<JsonObject>();
      for (JsonPair p : obj)
      {
        if (strcmp(p.key().c_str(), "systemsettings") == 0)
        {
          if (p.value().is<JsonObject>())
          {

            JsonObject value = p.value();
            if (systemConfig.set(value))
            {
              saveSystemConfigflag = true;
            }
          }
        }
        if (strcmp(p.key().c_str(), "logsettings") == 0)
        {
          if (p.value().is<JsonObject>())
          {

            JsonObject value = p.value();
            if (logConfig.set(value))
            {
              saveLogConfigflag = true;
            }
            setRFdebugLevel(logConfig.rfloglevel);
          }
        }
        if (strcmp(p.key().c_str(), "wifisettings") == 0)
        {
          if (p.value().is<JsonObject>())
          {

            JsonObject value = p.value();
            if (wifiConfig.set(value))
            {
              saveWifiConfigflag = true;
            }
          }
        }
      }
    }
    else
    {
      E_LOG("websocket: JSON parse failed, key:\"wifisettings\"");
    }
  }
  else if (msg.find("{\"button\"") != std::string::npos)
  {
    JsonDocument root;
    DeserializationError error = deserializeJson(root, msg);
    if (!error)
    {
      if (strcmp(root["button"].as<const char *>(), "restorepart") == 0)
      {
        repartition_device("legacy");
      }
      else if (strcmp(root["button"].as<const char *>(), "checkpart") == 0)
      {
        chkpartition = true;
      }
      else if (strcmp(root["button"].as<const char *>(), "i2cdebugon") == 0)
      {
        systemConfig.i2cmenu = 1;
        saveLogConfigflag = true;
      }
      else if (strcmp(root["button"].as<const char *>(), "i2cdebugoff") == 0)
      {
        systemConfig.i2cmenu = 0;
        saveLogConfigflag = true;
      }
      else if (strcmp(root["button"].as<const char *>(), "i2csnifferon") == 0)
      {
        systemConfig.i2c_sniffer = 1;
        saveLogConfigflag = true;
      }
      else if (strcmp(root["button"].as<const char *>(), "i2csnifferoff") == 0)
      {
        systemConfig.i2c_sniffer = 0;
        saveLogConfigflag = true;
      }
    }
    else
    {
      E_LOG("websocket: JSON parse failed, key:\"button\"");
    }
  }
  else if (msg.find("{\"ithobutton\"") != std::string::npos)
  {
    JsonDocument root;
    DeserializationError error = deserializeJson(root, msg);
    if (!error)
    {
      if (root["ithobutton"].as<const char *>() == nullptr)
      {
        uint16_t val = root["ithobutton"].as<uint16_t>();
        if (val == 2410)
        {
          i2c_queue_add_cmd([root]()
                            { getSetting(root["index"].as<uint8_t>(), false, true, false); });
        }
        else if (val == 24109)
        {
          if (ithoSettingsArray != nullptr)
          {
            uint8_t index = root["ithosetupdate"].as<uint8_t>();
            if (ithoSettingsArray[index].type == ithoSettings::is_float && ithoSettingsArray[index].length == 1)
            {
              updateSetting(root["ithosetupdate"].as<uint8_t>(), static_cast<int8_t>(root["value"].as<float>() * ithoSettingsArray[index].divider), true);
            }
            else if (ithoSettingsArray[index].type == ithoSettings::is_float && ithoSettingsArray[index].length == 2)
            {
              updateSetting(root["ithosetupdate"].as<uint8_t>(), static_cast<int16_t>(root["value"].as<float>() * ithoSettingsArray[index].divider), true);
            }
            else if (ithoSettingsArray[index].type == ithoSettings::is_float && ithoSettingsArray[index].length == 4)
            {
              updateSetting(root["ithosetupdate"].as<uint8_t>(), static_cast<int32_t>(root["value"].as<float>() * ithoSettingsArray[index].divider), true);
            }
            else
            {
              updateSetting(root["ithosetupdate"].as<uint8_t>(), root["value"].as<int32_t>(), true);
            }
          }
        }
        else if (val == 4210)
        {
          sendQueryCounters(true);
        }
        else if (val == 0xCE30)
        {
          setSettingCE30(root["ithotemptemp"].as<int16_t>(), root["ithotemp"].as<int16_t>(), root["ithotimestamp"].as<uint32_t>(), true);
        }
        else if (val == 0xC000)
        {
          sendCO2speed(root["itho_c000_speed1"].as<uint8_t>(), root["itho_c000_speed2"].as<uint8_t>());
        }
        else if (val == 0x9298)
        {
          sendCO2value(root["itho_9298_val"].as<uint16_t>());
        }
        else if (val == 4030)
        {
          setSetting4030(root["idx"].as<uint16_t>(), root["dt"].as<uint8_t>(), root["val"].as<int16_t>(), root["chk"].as<uint8_t>(), true);
        }
      }
      else
      {
        i2c_result_updateweb = true;
        ithoI2CCommand(0, root["ithobutton"], WEB);
      }
    }
    else
    {
      E_LOG("websocket: JSON parse failed, key:\"ithobutton\"");
    }
  }
  else if (msg.find("{\"wifisetup\"") != std::string::npos)
  {
    jsonWsSend("wifisettings");
  }
  else if (msg.find("{\"wifistat\"") != std::string::npos)
  {
    jsonWsSend("wifistat");
  }
  else if (msg.find("{\"syssetup\"") != std::string::npos)
  {
    systemConfig.get_sys_settings = true;
    jsonWsSend("systemsettings");
  }
  else if (msg.find("{\"logsetup\"") != std::string::npos)
  {
    jsonWsSend("logsettings");
  }
  else if (msg.find("{\"mqttsetup\"") != std::string::npos)
  {
    systemConfig.get_mqtt_settings = true;
    jsonWsSend("systemsettings");
    sysStatReq = true;
  }
  else if (msg.find("{\"rfsetup\"") != std::string::npos)
  {
    systemConfig.get_rf_settings = true;
    jsonWsSend("systemsettings");
  }
  else if (msg.find("{\"ithosetup\"") != std::string::npos)
  {
    jsonWsSend("ithodevinfo");
    sysStatReq = true;
  }
  else if (msg.find("{\"ithostatus\"") != std::string::npos)
  {
    jsonWsSend("ithostatusinfo");
    sysStatReq = true;
  }
  else if (msg.find("{\"ithogetsetting\"") != std::string::npos)
  {
    JsonDocument root;
    DeserializationError error = deserializeJson(root, msg.c_str());
    if (!error)
    {
      // step1: index=0, update=false, loop=true -> reply with settings labels but null for values
      i2c_queue_add_cmd([root]()
                        { getSetting(root["index"].as<uint8_t>(), root["update"].as<bool>(), false, true); });
    }
    else
    {
      E_LOG("websocket: JSON parse failed, key:\"ithogetsetting\"");
    }
  }
  else if (msg.find("{\"ithosetrefresh\"") != std::string::npos)
  {
    JsonDocument root;
    DeserializationError error = deserializeJson(root, msg.c_str());
    if (!error)
    {
      i2c_queue_add_cmd([root]()
                        { getSetting(root["ithosetrefresh"].as<uint8_t>(), true, false, false); });
    }
    else
    {
      E_LOG("websocket: JSON parse failed, key:\"ithosetrefresh\"");
    }
  }
  else if (msg.find("{\"ithosetupdate\"") != std::string::npos)
  {
    JsonDocument root;
    DeserializationError error = deserializeJson(root, msg.c_str());
    if (!error)
    {
      if (ithoSettingsArray != nullptr)
      {
        uint8_t index = root["ithosetupdate"].as<uint8_t>();
        if (ithoSettingsArray[index].type == ithoSettings::is_float && ithoSettingsArray[index].length == 1)
        {
          updateSetting(root["ithosetupdate"].as<uint8_t>(), static_cast<int8_t>(root["value"].as<float>() * ithoSettingsArray[index].divider), false);
        }
        else if (ithoSettingsArray[index].type == ithoSettings::is_float && ithoSettingsArray[index].length == 2)
        {
          updateSetting(root["ithosetupdate"].as<uint8_t>(), static_cast<int16_t>(root["value"].as<float>() * ithoSettingsArray[index].divider), false);
        }
        else if (ithoSettingsArray[index].type == ithoSettings::is_float && ithoSettingsArray[index].length == 4)
        {
          updateSetting(root["ithosetupdate"].as<uint8_t>(), static_cast<int32_t>(root["value"].as<float>() * ithoSettingsArray[index].divider), false);
        }
        else
        {
          updateSetting(root["ithosetupdate"].as<uint8_t>(), root["value"].as<int32_t>(), false);
        }
      }
    }
    else
    {
      E_LOG("websocket: JSON parse failed, key:\"ithosetupdate\"");
    }
  }
  else if (msg.find("{\"ithoremotes\"") != std::string::npos)
  {
    jsonWsSend("remotes");
    sysStatReq = true;
  }
  else if (msg.find("{\"ithovremotes\"") != std::string::npos)
  {
    jsonWsSend("vremotes");
    sysStatReq = true;
  }
  else if (msg.find("{\"vremote\"") != std::string::npos)
  {
    JsonDocument root;
    DeserializationError error = deserializeJson(root, msg.c_str());
    if (!error)
    {
      if (!root["vremote"].isNull() && !root["command"].isNull())
      {
        ithoI2CCommand(root["vremote"].as<uint8_t>(), root["command"].as<const char *>(), WEB);
      }
    }
    else
    {
      E_LOG("websocket: JSON parse failed, key:\"vremote\"");
    }
  }
  else if (msg.find("{\"remote\"") != std::string::npos)
  {
    JsonDocument root;
    DeserializationError error = deserializeJson(root, msg.c_str());
    if (!error)
    {
      if (!root["remote"].isNull() && !root["command"].isNull())
      {
        ithoExecRFCommand(root["remote"].as<uint8_t>(), root["command"].as<const char *>(), WEB);
      }
    }
    else
    {
      E_LOG("websocket: JSON parse failed, key:\"remote\"");
    }
  }
  else if (msg.find("{\"command\"") != std::string::npos)
  {
    JsonDocument root;
    DeserializationError error = deserializeJson(root, msg.c_str());
    if (!error)
    {
      if (!root["command"].isNull())
      {
        ithoExecCommand(root["command"].as<const char *>(), WEB);
      }
    }
    else
    {
      E_LOG("websocket: JSON parse failed, key:\"command\"");
    }
  }

  else if (msg.find("{\"itho_llm\"") != std::string::npos)
  {
    toggleRemoteLLmode("remote");
  }
  else if (msg.find("{\"copy_id\"") != std::string::npos)
  {
    JsonDocument root;
    DeserializationError error = deserializeJson(root, msg.c_str());
    if (!error)
    {
      bool parseOK = false;
      int number = root["index"];
      if (number > 0 && number < virtualRemotes.getMaxRemotes() + 1)
      {
        number--; //+1 was added earlier to offset index from 0
        parseOK = true;
      }
      if (parseOK)
      {
        virtualRemotes.copy_id_remote_idx = number;
        toggleRemoteLLmode("vremote");
      }
    }
  }
  else if (msg.find("{\"itho_remove_remote\"") != std::string::npos)
  {
    JsonDocument root;
    DeserializationError error = deserializeJson(root, msg.c_str());
    if (!error)
    {
      bool parseOK = false;
      int index = root["itho_remove_remote"];
      if (index > 0 && index < remotes.getMaxRemotes() + 1)
      {
        parseOK = true;
        index--; //+1 was added earlier to offset index from 0
      }
      if (parseOK)
      {
        remotes.removeRemote(index);
        saveRemotesflag = true;
      }
    }
    else
    {
      E_LOG("websocket: JSON parse failed, key:\"itho_remove_remote\"");
    }
  }
  else if (msg.find("{\"itho_update_remote\"") != std::string::npos)
  {
    JsonDocument root;
    DeserializationError error = deserializeJson(root, msg.c_str());
    if (!error)
    {
      uint8_t index = root["itho_update_remote"].as<unsigned int>();
      remotes.updateRemoteName(index, root["value"] | "");
      RemoteFunctions remfunc = static_cast<RemoteFunctions>(root["remfunc"].as<uint8_t>() | 0);
      remotes.updateRemoteFunction(index, remfunc);
      RemoteTypes type = static_cast<RemoteTypes>(root["remtype"].as<uint16_t>() | 0);
      remotes.updateRemoteType(index, type);
      bool bidirectional = root["bidirectional"] | 0;
      remotes.updateRemoteBidirectional(index, bidirectional);
      // bool bidirectional = (type == RemoteTypes::RFTAUTON || type == RemoteTypes::RFTN || type == RemoteTypes::RFTCO2 || type == RemoteTypes::RFTRV || type == RemoteTypes::RFTSPIDER) ? (remfunc != RemoteFunctions::MONITOR ? true : false) : false;
      uint8_t id[3] = {0, 0, 0};
      id[0] = root["id"][0].as<uint8_t>();
      id[1] = root["id"][1].as<uint8_t>();
      id[2] = root["id"][2].as<uint8_t>();
      remotes.updateRemoteID(index, id[0], id[1], id[2]);
      rf.updateRFDevice(index, id[0], id[1], id[2], remotes.getRemoteType(index), bidirectional);
      saveRemotesflag = true;
    }
    else
    {
      E_LOG("websocket: JSON parse failed, key:\"itho_update_remote\"");
    }
  }
  else if (msg.find("{\"itho_remove_vremote\"") != std::string::npos)
  {
    JsonDocument root;
    DeserializationError error = deserializeJson(root, msg.c_str());
    if (!error)
    {
      bool parseOK = false;
      int index = root["itho_remove_vremote"];
      if (index > 0 && index < virtualRemotes.getMaxRemotes() + 1)
      {
        parseOK = true;
        index--; //+1 was added earlier to offset index from 0
      }
      if (parseOK)
      {
        virtualRemotes.removeRemote(index);
        saveVremotesflag = true;
      }
    }
    else
    {
      E_LOG("websocket: JSON parse failed, key:\"itho_remove_vremote\"");
    }
  }
  else if (msg.find("{\"itho_update_vremote\"") != std::string::npos)
  {
    JsonDocument root;
    DeserializationError error = deserializeJson(root, msg.c_str());
    if (!error)
    {
      uint8_t index = root["itho_update_vremote"].as<unsigned int>();
      virtualRemotes.updateRemoteName(index, root["value"] | "");
      virtualRemotes.updateRemoteType(index, root["remtype"] | 0);
      uint8_t ID[3] = {0, 0, 0};
      ID[0] = root["id"][0].as<uint8_t>();
      ID[1] = root["id"][1].as<uint8_t>();
      ID[2] = root["id"][2].as<uint8_t>();
      virtualRemotes.updateRemoteID(index, ID[0], ID[1], ID[2]);
      saveVremotesflag = true;
    }
    else
    {
      E_LOG("websocket: JSON parse failed, key:\"itho_update_vremote\"");
    }
  }
  else if (msg.find("{\"update_rf_id\"") != std::string::npos)
  {
    JsonDocument root;
    DeserializationError error = deserializeJson(root, msg.c_str());
    if (!error)
    {
      uint8_t ID[3] = {0, 0, 0};
      ID[0] = root["update_rf_id"][0].as<uint8_t>();
      ID[1] = root["update_rf_id"][1].as<uint8_t>();
      ID[2] = root["update_rf_id"][2].as<uint8_t>();

      if (ID[0] == 0 && ID[1] == 0 && ID[2] == 0)
      {
        systemConfig.module_rf_id[0] = sys.getMac(3);
        systemConfig.module_rf_id[1] = sys.getMac(4);
        systemConfig.module_rf_id[2] = sys.getMac(5) - 1;
      }
      else
      {
        systemConfig.module_rf_id[0] = ID[0];
        systemConfig.module_rf_id[1] = ID[1];
        systemConfig.module_rf_id[2] = ID[2];
      }
      rf.setDefaultID(systemConfig.module_rf_id[0], systemConfig.module_rf_id[1], systemConfig.module_rf_id[2]);

      saveSystemConfigflag = true;
    }
    else
    {
      E_LOG("websocket: JSON parse failed, key:\"update_rf_id\"");
    }
  }
  else if (msg.find("{\"reboot\"") != std::string::npos)
  {
    JsonDocument root;
    DeserializationError error = deserializeJson(root, msg.c_str());
    if (!error)
    {
      shouldReboot = root["reboot"];
      dontSaveConfig = root["dontsaveconf"];
    }
  }
  else if (msg.find("{\"resetwificonf\"") != std::string::npos)
  {
    resetWifiConfigflag = true;
  }
  else if (msg.find("{\"resetsysconf\"") != std::string::npos)
  {
    resetSystemConfigflag = true;
  }
  else if (msg.find("{\"format\"") != std::string::npos)
  {
    formatFileSystem = true;
  }
  else if (msg.find("{\"itho\"") != std::string::npos)
  {
    JsonDocument root;
    DeserializationError error = deserializeJson(root, msg);
    if (!error)
    {
      ithoSetSpeed(root["itho"].as<uint16_t>(), WEB);
    }
    else
    {
      E_LOG("websocket: JSON parse failed, key:\"itho\"");
    }
  }
  else if (msg.find("{\"rfdebug\"") != std::string::npos)
  {
    JsonDocument root;
    DeserializationError error = deserializeJson(root, msg.c_str());
    if (!error)
    {
      if (root["rfdebug"].as<uint16_t>() == 0x31DA)
      {
        faninfo31DA = root["faninfo"].as<uint8_t>();
        timer31DA = root["timer"].as<uint8_t>();
        send31DAdebug = true;
      }
      else if (root["rfdebug"].as<uint16_t>() == 0x31D9)
      {
        status31D9 = root["status"].as<uint8_t>() * 2;
        fault31D9 = root["fault"].as<bool>();
        frost31D9 = root["frost"].as<bool>();
        filter31D9 = root["filter"].as<bool>();
        send31D9debug = true;
      }
    }
  }
  else if (msg.find("{\"i2csniffer\"") != std::string::npos)
  {
    JsonDocument root;
    DeserializationError error = deserializeJson(root, msg.c_str());
    if (!error)
    {
      if (root["i2csniffer"].as<uint8_t>() == 1)
      {
        i2c_safe_guard.sniffer_enabled = true;
        i2c_safe_guard.sniffer_web_enabled = true;
        i2c_sniffer_enable();
      }
      else
      {
        i2c_safe_guard.sniffer_enabled = false;
        i2c_safe_guard.sniffer_web_enabled = false;
        i2c_sniffer_disable();
      }
    }
  }
}
#if defined MG_ENABLE_PACKED_FS && MG_ENABLE_PACKED_FS == 1
static void wsEvent(struct mg_connection *c, int ev, void *ev_data)
{
  if (ev == MG_EV_HTTP_MSG)
  {
    struct mg_http_message *hm = (struct mg_http_message *)ev_data;
    if (mg_match(hm->uri, mg_str("/ws"), nullptr))
    {
      if (systemConfig.syssec_web && !webauth_ok)
      {
        return;
      }
      // Upgrade to websocket. From now on, a connection is a full-duplex
      // Websocket connection, which will receive MG_EV_WS_MSG events.
      mg_ws_upgrade(c, hm, NULL);
    }
  }
  else if (ev == MG_EV_WS_MSG)
  {
    if (systemConfig.syssec_web && !webauth_ok)
    {
      return;
    }
    // Got websocket frame. Received data is wm->data.
    struct mg_ws_message *wm = (struct mg_ws_message *)ev_data;
    std::string msg;
    msg.assign(wm->data.buf, wm->data.len);

    handle_ws_message(std::move(msg));

    wm = nullptr;
    // msg = std::string();
  }
}
#else
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT)
  {
    client->setCloseClientOnQueueFull(false);
    return;
  }
  else if (type == WS_EVT_DISCONNECT)
  {
    // Serial.printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id(), client->id());
  }
  else if (type == WS_EVT_ERROR)
  {
    // Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  }
  else if (type == WS_EVT_PONG)
  {
    // Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
  }
  else if (type == WS_EVT_DATA)
  {
    std::string msg;
    AwsFrameInfo *info = (AwsFrameInfo *)arg;

    if (info->final && info->index == 0 && info->len == len)
    {
      if (info->opcode == WS_TEXT)
      {
        for (size_t i = 0; i < info->len; i++)
        {
          msg += (char)data[i];
        }
      }
      else
      {
        char buff[4];
        for (size_t i = 0; i < info->len; i++)
        {
          sprintf(buff, "%02x ", (uint8_t)data[i]);
          msg += buff;
        }
      }
    }
    if (strcmp(msg.c_str(), "ping") == 0)
    {
      client->text("pong");
    }
    else
    {
      handle_ws_message(std::move(msg));
    }
  }
}
#endif