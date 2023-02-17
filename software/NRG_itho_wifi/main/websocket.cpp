#include "websocket.h"

static void wsEvent(struct mg_connection *c, int ev, void *ev_data, void *fn_data);

void websocketInit()
{
  mg_log_set(0);
  mg_mgr_init(&mgr);                                   // Initialise event manager
  mg_http_listen(&mgr, s_listen_on_ws, wsEvent, &mgr); // Create WS listener
}

void jsonWsSend(const char *rootName)
{
  DynamicJsonDocument root(4000);

  if (strcmp(rootName, "wifisettings") == 0)
  {
    JsonObject nested = root.createNestedObject(rootName);
    nested["ssid"] = wifiConfig.ssid;
    nested["passwd"] = wifiConfig.passwd;
    nested["dhcp"] = wifiConfig.dhcp;
    if (strcmp(wifiConfig.dhcp, "off") == 0)
    {
      nested["ip"] = wifiConfig.ip;
      nested["subnet"] = wifiConfig.subnet;
      nested["gateway"] = wifiConfig.gateway;
      nested["dns1"] = wifiConfig.dns1;
      nested["dns2"] = wifiConfig.dns2;
    }
    else
    {
      nested["ip"] = WiFi.localIP().toString();
      nested["subnet"] = WiFi.subnetMask().toString();
      nested["gateway"] = WiFi.gatewayIP().toString();
      nested["dns1"] = WiFi.dnsIP().toString();
      nested["dns2"] = WiFi.dnsIP(1).toString();
    }
    nested["port"] = wifiConfig.port;
    nested["hostname"] = hostName();
    nested["ntpserver"] = wifiConfig.ntpserver;
  }
  else if (strcmp(rootName, "systemsettings") == 0)
  {
    JsonObject nested = root.createNestedObject(rootName);
    systemConfig.get(nested);
  }
  else if (strcmp(rootName, "logsettings") == 0)
  {
    JsonObject nested = root.createNestedObject(rootName);
    logConfig.get(nested);
    if (prevlog_available())
    {
      nested["prevlog"] = "/prevlog";
    }
  }
  else if (strcmp(rootName, "ithodevinfo") == 0)
  {
    JsonObject nested = root.createNestedObject(rootName);
    nested["itho_devtype"] = getIthoType();
    nested["itho_fwversion"] = currentItho_fwversion();
    nested["itho_setlen"] = currentIthoSettingsLength();
  }
  else if (strcmp(rootName, "ithostatusinfo") == 0)
  {
    JsonObject nested = root.createNestedObject(rootName);
    getIthoStatusJSON(nested);
  }
  else if (strcmp(rootName, "debuginfo") == 0)
  {
    JsonObject nested = root.createNestedObject(rootName);
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
    JsonObject nested = root.createNestedObject(rootName);
    nested["remtype"] = static_cast<uint16_t>(virtualRemotes.getRemoteType(0));
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
  notifyClients(root.as<JsonObjectConst>());
}

static void wsEvent(struct mg_connection *c, int ev, void *ev_data, void *fn_data)
{
  if (ev == MG_EV_HTTP_MSG)
  {
    struct mg_http_message *hm = (struct mg_http_message *)ev_data;
    if (mg_http_match_uri(hm, "/ws"))
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
    msg.assign(wm->data.ptr, wm->data.len);

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
      DynamicJsonDocument root(2048);
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
    }
    else if (msg.find("{\"button\"") != std::string::npos)
    {
      StaticJsonDocument<128> root;
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
      }
    }
    else if (msg.find("{\"ithobutton\"") != std::string::npos)
    {
      StaticJsonDocument<128> root;
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
                updateSetting(root["ithosetupdate"].as<uint8_t>(), static_cast<int8_t>(root["value"].as<double>() * ithoSettingsArray[index].divider), true);
              }
              else if (ithoSettingsArray[index].type == ithoSettings::is_float && ithoSettingsArray[index].length == 2)
              {
                updateSetting(root["ithosetupdate"].as<uint8_t>(), static_cast<int16_t>(root["value"].as<double>() * ithoSettingsArray[index].divider), true);
              }
              else if (ithoSettingsArray[index].type == ithoSettings::is_float && ithoSettingsArray[index].length == 4)
              {
                updateSetting(root["ithosetupdate"].as<uint8_t>(), static_cast<int32_t>(root["value"].as<double>() * ithoSettingsArray[index].divider), true);
              }
              else
              {
                updateSetting(root["ithosetupdate"].as<uint8_t>(), root["value"].as<int32_t>(), true);
              }
            }
          }
          else if (val == 0xCE30)
          {
            setSettingCE30(root["ithotemptemp"].as<int16_t>(), root["ithotemp"].as<int16_t>(), root["ithotimestamp"].as<uint32_t>(), true);
          }
        }
        else
        {
          i2c_result_updateweb = true;
          ithoI2CCommand(0, root["ithobutton"], WEB);
        }
      }
    }
    else if (msg.find("{\"wifisetup\"") != std::string::npos)
    {
      jsonWsSend("wifisettings");
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
      StaticJsonDocument<128> root;
      DeserializationError error = deserializeJson(root, msg.c_str());
      if (!error)
      {
        // step1: index=0, update=false, loop=true -> reply with settings labels but null for values
        i2c_queue_add_cmd([root]()
                          { getSetting(root["index"].as<uint8_t>(), root["update"].as<bool>(), false, true); });
      }
    }
    else if (msg.find("{\"ithosetrefresh\"") != std::string::npos)
    {
      StaticJsonDocument<128> root;
      DeserializationError error = deserializeJson(root, msg.c_str());
      if (!error)
      {
        i2c_queue_add_cmd([root]()
                          { getSetting(root["ithosetrefresh"].as<uint8_t>(), true, false, false); });
      }
    }
    else if (msg.find("{\"ithosetupdate\"") != std::string::npos)
    {
      StaticJsonDocument<128> root;
      DeserializationError error = deserializeJson(root, msg.c_str());
      if (!error)
      {
        if (ithoSettingsArray != nullptr)
        {
          uint8_t index = root["ithosetupdate"].as<uint8_t>();
          if (ithoSettingsArray[index].type == ithoSettings::is_float && ithoSettingsArray[index].length == 1)
          {
            updateSetting(root["ithosetupdate"].as<uint8_t>(), static_cast<int8_t>(root["value"].as<double>() * ithoSettingsArray[index].divider), false);
          }
          else if (ithoSettingsArray[index].type == ithoSettings::is_float && ithoSettingsArray[index].length == 2)
          {
            updateSetting(root["ithosetupdate"].as<uint8_t>(), static_cast<int16_t>(root["value"].as<double>() * ithoSettingsArray[index].divider), false);
          }
          else if (ithoSettingsArray[index].type == ithoSettings::is_float && ithoSettingsArray[index].length == 4)
          {
            updateSetting(root["ithosetupdate"].as<uint8_t>(), static_cast<int32_t>(root["value"].as<double>() * ithoSettingsArray[index].divider), false);
          }
          else
          {
            updateSetting(root["ithosetupdate"].as<uint8_t>(), root["value"].as<int32_t>(), false);
          }
        }
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
      StaticJsonDocument<128> root;
      DeserializationError error = deserializeJson(root, msg.c_str());
      if (!error)
      {
        if (!root["vremote"].isNull() && !root["command"].isNull())
        {
          ithoI2CCommand(root["vremote"].as<uint8_t>(), root["command"].as<const char *>(), WEB);
        }
      }
    }
    else if (msg.find("{\"command\"") != std::string::npos)
    {
      StaticJsonDocument<128> root;
      DeserializationError error = deserializeJson(root, msg.c_str());
      if (!error)
      {
        if (!root["command"].isNull())
        {
          ithoExecCommand(root["command"].as<const char *>(), WEB);
        }
      }
    }

    else if (msg.find("{\"itho_llm\"") != std::string::npos)
    {
      toggleRemoteLLmode("remote");
    }
    else if (msg.find("{\"copy_id\"") != std::string::npos)
    {
      StaticJsonDocument<128> root;
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
          virtualRemotes.activeRemote = number;
          toggleRemoteLLmode("vremote");
        }
      }
    }
    else if (msg.find("{\"itho_remove_remote\"") != std::string::npos)
    {
      StaticJsonDocument<128> root;
      DeserializationError error = deserializeJson(root, msg.c_str());
      if (!error)
      {
        bool parseOK = false;
        int number = root["itho_remove_remote"];
        if (number > 0 && number < remotes.getMaxRemotes() + 1)
        {
          parseOK = true;
          number--; //+1 was added earlier to offset index from 0
        }
        if (parseOK)
        {
          remotes.removeRemote(number);
          const int *id = remotes.getRemoteIDbyIndex(number);
          rf.setBindAllowed(true);
          rf.removeRFDevice(*id, *(id + 1), *(id + 2));
          rf.setBindAllowed(false);
          saveRemotesflag = true;
        }
      }
    }
    else if (msg.find("{\"itho_update_remote\"") != std::string::npos)
    {
      StaticJsonDocument<512> root;
      DeserializationError error = deserializeJson(root, msg.c_str());
      if (!error)
      {
        uint8_t index = root["itho_update_remote"].as<unsigned int>();
        remotes.updateRemoteName(index, root["value"] | "");
        remotes.updateRemoteFunction(index, root["remtype"] | 0);
        uint8_t id[3] = {0, 0, 0};
        id[0] = root["id"][0].as<uint8_t>();
        id[1] = root["id"][1].as<uint8_t>();
        id[2] = root["id"][2].as<uint8_t>();
        rf.setBindAllowed(true);
        const int *current_id = remotes.getRemoteIDbyIndex(index);
        rf.removeRFDevice(*current_id, *(current_id + 1), *(current_id + 2));
        remotes.updateRemoteID(index, id);
        rf.addRFDevice(*id, *(id + 1), *(id + 2), remotes.getRemoteType(index));
        rf.setBindAllowed(false);
        saveRemotesflag = true;
      }
    }
    else if (msg.find("{\"itho_remove_vremote\"") != std::string::npos)
    {
      StaticJsonDocument<128> root;
      DeserializationError error = deserializeJson(root, msg.c_str());
      if (!error)
      {
        bool parseOK = false;
        int number = root["itho_remove_vremote"];
        if (number > 0 && number < virtualRemotes.getMaxRemotes() + 1)
        {
          parseOK = true;
          number--; //+1 was added earlier to offset index from 0
        }
        if (parseOK)
        {
          virtualRemotes.removeRemote(number);
          saveVremotesflag = true;
        }
      }
    }
    else if (msg.find("{\"itho_update_vremote\"") != std::string::npos)
    {
      StaticJsonDocument<512> root;
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
        virtualRemotes.updateRemoteID(index, ID);
        saveVremotesflag = true;
      }
    }
    else if (msg.find("{\"reboot\"") != std::string::npos)
    {
      StaticJsonDocument<128> root;
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
      StaticJsonDocument<128> root;
      DeserializationError error = deserializeJson(root, msg);
      if (!error)
      {
        ithoSetSpeed(root["itho"].as<uint16_t>(), WEB);
      }
    }
    else if (msg.find("{\"rfdebug\"") != std::string::npos)
    {
      StaticJsonDocument<128> root;
      DeserializationError error = deserializeJson(root, msg.c_str());
      if (!error)
      {
        setRFdebugLevel(root["rfdebug"].as<uint8_t>());
      }
    }
    else if (msg.find("{\"i2csniffer\"") != std::string::npos)
    {
      StaticJsonDocument<128> root;
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

    wm = nullptr;
    msg = std::string();
  }
  (void)fn_data;
}
