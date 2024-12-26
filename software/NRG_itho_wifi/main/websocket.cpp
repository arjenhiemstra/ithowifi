#include "websocket.h"

#if defined MG_ENABLE_PACKED_FS && MG_ENABLE_PACKED_FS == 1
#else
static std::unordered_map<uint32_t, std::string> g_wsBuffers;
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
  else if (strcmp(rootName, "hadiscinfo") == 0)
  {
    JsonObject nested = root["ithostatusinfo"].to<JsonObject>();
    uint8_t count = getIthoStatusJSON(nested);
    root["count"] = count;
    root["target"] = "hadisc";
    root["itho_status_ready"] = itho_status_ready();
    root["iis"] = itho_init_status;
  }
  else if (strcmp(rootName, "hadiscsettings") == 0)
  {
    D_LOG("hadiscsettings request recieved2");
    JsonObject nested = root[rootName].to<JsonObject>();
    haDiscConfig.get(nested);
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

void handle_ws_message(JsonObject root)
{
  // D_LOG("%s", msg.c_str());

  if (root["wifiscan"].is<bool>() && root["wifiscan"].as<bool>())
  {
    D_LOG("Start wifi scan");
    runscan = true;
  }
  if (root["sysstat"].is<bool>() && root["sysstat"].as<bool>())
  {
    sysStatReq = true;
  }
  if (root["remtype"].is<bool>() && root["remtype"].as<bool>())
  {
    jsonWsSend("remtypeconf");
  }

  // ---------- "debugvalues" vs. "i2cdebuglog" ----------
  if (root["debugvalues"].is<bool>() && root["debugvalues"].as<bool>())
  {
    jsonWsSend("debuginfo");
  }
  else if (root["i2cdebuglog"].is<bool>() && root["i2cdebuglog"].as<bool>())
  {
    jsonWsSend("i2cdebuglog");
  }

  // ---------- WiFi/system/log/hadisc config ----------
  if (root["wifisettings"].is<JsonObject>())
  {
    if (wifiConfig.set(root["wifisettings"].as<JsonObject>()))
    {
      saveWifiConfigflag = true;
    }
  }
  if (root["systemsettings"].is<JsonObject>())
  {
    if (systemConfig.set(root["systemsettings"].as<JsonObject>()))
    {
      saveSystemConfigflag = true;
    }
  }
  if (root["logsettings"].is<JsonObject>())
  {
    if (logConfig.set(root["logsettings"].as<JsonObject>()))
    {
      saveLogConfigflag = true;
    }
    setRFdebugLevel(logConfig.rfloglevel);
  }
  if (root["hadiscsettings"].is<JsonObject>())
  {
    D_LOG("hadiscsettings received");
    if (haDiscConfig.set(root["hadiscsettings"].as<JsonObject>()))
    {
      saveHADiscConfigflag = true;
    }
    else
    {
      E_LOG("hadiscsettings: set config failed");
    }
  }

  // ---------- "button" (string) ----------
  if (root["button"].is<const char *>())
  {
    const char *btn = root["button"].as<const char *>();
    if (strcmp(btn, "restorepart") == 0)
    {
      repartition_device("legacy");
    }
    else if (strcmp(btn, "checkpart") == 0)
    {
      chkpartition = true;
    }
    else if (strcmp(btn, "i2cdebugon") == 0)
    {
      systemConfig.i2cmenu = 1;
      saveSystemConfigflag = true;
    }
    else if (strcmp(btn, "i2cdebugoff") == 0)
    {
      systemConfig.i2cmenu = 0;
      saveSystemConfigflag = true;
    }
    else if (strcmp(btn, "i2csnifferon") == 0)
    {
      systemConfig.i2c_sniffer = 1;
      saveSystemConfigflag = true;
    }
    else if (strcmp(btn, "i2csnifferoff") == 0)
    {
      systemConfig.i2c_sniffer = 0;
      saveSystemConfigflag = true;
    }
  }

  // ---------- "ithobutton": can be a string or an integer ----------
  if (root["ithobutton"].is<const char *>())
  {
    i2c_result_updateweb = true;
    ithoI2CCommand(0, root["ithobutton"].as<const char *>(), WEB);
  }
  else if (root["ithobutton"].is<uint16_t>())
  {
    uint16_t val = root["ithobutton"].as<uint16_t>();
    switch (val)
    {
    case 2410:
    {
      if (root["index"].is<uint8_t>())
      {
        uint8_t idx = root["index"].as<uint8_t>();
        i2c_queue_add_cmd([idx]()
                          { getSetting(idx, false, true, false); });
      }
      break;
    }
    case 24109:
    {
      if (ithoSettingsArray &&
          root["ithosetupdate"].is<uint8_t>() &&
          root["value"].is<float>())
      {
        uint8_t index = root["ithosetupdate"].as<uint8_t>();
        float fval = root["value"].as<float>();
        auto &entry = ithoSettingsArray[index];
        if (entry.type == ithoSettings::is_float)
        {
          if (entry.length == 1)
            updateSetting(index, (int8_t)(fval * entry.divider), true);
          else if (entry.length == 2)
            updateSetting(index, (int16_t)(fval * entry.divider), true);
          else if (entry.length == 4)
            updateSetting(index, (int32_t)(fval * entry.divider), true);
          else
            updateSetting(index, (int32_t)fval, true);
        }
        else
        {
          updateSetting(index, (int32_t)fval, true);
        }
      }
      break;
    }
    case 4210:
      sendQueryCounters(true);
      break;
    case 0xCE30:
      if (root["ithotemptemp"].is<int16_t>() &&
          root["ithotemp"].is<int16_t>() &&
          root["ithotimestamp"].is<uint32_t>())
      {
        setSettingCE30(root["ithotemptemp"].as<int16_t>(),
                       root["ithotemp"].as<int16_t>(),
                       root["ithotimestamp"].as<uint32_t>(),
                       true);
      }
      break;
    case 0xC000:
      if (root["itho_c000_speed1"].is<uint8_t>() &&
          root["itho_c000_speed2"].is<uint8_t>())
      {
        sendCO2speed(root["itho_c000_speed1"].as<uint8_t>(),
                     root["itho_c000_speed2"].as<uint8_t>());
      }
      break;
    case 0x9298:
      if (root["itho_9298_val"].is<uint16_t>())
      {
        sendCO2value(root["itho_9298_val"].as<uint16_t>());
      }
      break;
    case 4030:
      if (root["idx"].is<uint16_t>() &&
          root["dt"].is<uint8_t>() &&
          root["val"].is<int16_t>() &&
          root["chk"].is<uint8_t>())
      {
        setSetting4030(root["idx"].as<uint16_t>(),
                       root["dt"].as<uint8_t>(),
                       root["val"].as<int16_t>(),
                       root["chk"].as<uint8_t>(),
                       true);
      }
      break;
    }
  }

  // ---------- Quick config queries ----------
  if (root["wifisetup"].is<bool>() && root["wifisetup"].as<bool>())
  {
    jsonWsSend("wifisettings");
  }
  if (root["wifistat"].is<bool>() && root["wifistat"].as<bool>())
  {
    jsonWsSend("wifistat");
  }
  if (root["syssetup"].is<bool>() && root["syssetup"].as<bool>())
  {
    systemConfig.get_sys_settings = true;
    jsonWsSend("systemsettings");
  }
  if (root["logsetup"].is<bool>() && root["logsetup"].as<bool>())
  {
    jsonWsSend("logsettings");
  }
  if (root["mqttsetup"].is<bool>() && root["mqttsetup"].as<bool>())
  {
    systemConfig.get_mqtt_settings = true;
    jsonWsSend("systemsettings");
    sysStatReq = true;
  }
  if (root["rfsetup"].is<bool>() && root["rfsetup"].as<bool>())
  {
    systemConfig.get_rf_settings = true;
    jsonWsSend("systemsettings");
  }
  if (root["ithosetup"].is<bool>() && root["ithosetup"].as<bool>())
  {
    jsonWsSend("ithodevinfo");
    sysStatReq = true;
  }
  if (root["ithostatus"].is<bool>() && root["ithostatus"].as<bool>())
  {
    jsonWsSend("ithostatusinfo");
    sysStatReq = true;
  }
  if (root["hadiscinfo"].is<bool>() && root["hadiscinfo"].as<bool>())
  {
    jsonWsSend("hadiscinfo");
    sysStatReq = true;
  }
  if (root["gethadiscsettings"].is<bool>() && root["gethadiscsettings"].as<bool>())
  {
    jsonWsSend("hadiscsettings");
    sysStatReq = true;
  }

  // ---------- Itho get/refresh/update settings ----------
  if (root["ithogetsetting"].is<JsonObject>())
  {
    auto setObj = root["ithogetsetting"].as<JsonObject>();
    uint8_t idx = setObj["index"] | 0;
    bool upd = setObj["update"] | false;
    i2c_queue_add_cmd([idx, upd]()
                      { getSetting(idx, upd, false, true); });
  }
  if (root["ithosetrefresh"].is<JsonObject>())
  {
    auto refObj = root["ithosetrefresh"].as<JsonObject>();
    uint8_t idx = refObj["ithosetrefresh"] | 0;
    i2c_queue_add_cmd([idx]()
                      { getSetting(idx, true, false, false); });
  }
  if (root["ithosetupdate"].is<uint8_t>() && root["value"].is<float>())
  {
    if (ithoSettingsArray)
    {
      uint8_t index = root["ithosetupdate"].as<uint8_t>();
      float fval = root["value"].as<float>();
      auto &entry = ithoSettingsArray[index];
      if (entry.type == ithoSettings::is_float)
      {
        if (entry.length == 1)
          updateSetting(index, (int8_t)(fval * entry.divider), false);
        else if (entry.length == 2)
          updateSetting(index, (int16_t)(fval * entry.divider), false);
        else if (entry.length == 4)
          updateSetting(index, (int32_t)(fval * entry.divider), false);
        else
          updateSetting(index, (int32_t)fval, false);
      }
      else
      {
        updateSetting(index, (int32_t)fval, false);
      }
    }
  }

  // ---------- Itho remote queries ----------
  if (root["ithoremotes"].is<bool>() && root["ithoremotes"].as<bool>())
  {
    jsonWsSend("remotes");
    sysStatReq = true;
  }
  if (root["ithovremotes"].is<bool>() && root["ithovremotes"].as<bool>())
  {
    jsonWsSend("vremotes");
    sysStatReq = true;
  }

  // "vremote" => use ithoI2CCommand
  if (root["vremote"].is<uint8_t>() && root["command"].is<const char *>())
  {
    ithoI2CCommand(root["vremote"].as<uint8_t>(), root["command"].as<const char *>(), WEB);
  }
  // "remote" => use ithoExecRFCommand
  if (root["remote"].is<uint8_t>() && root["command"].is<const char *>())
  {
    ithoExecRFCommand(root["remote"].as<uint8_t>(), root["command"].as<const char *>(), WEB);
  }
  // Single top-level "command"
  if (root["command"].is<const char *>())
  {
    ithoExecCommand(root["command"].as<const char *>(), WEB);
  }
  // Itho speed command
  if (root["itho"].is<uint16_t>())
  {
    ithoSetSpeed(root["itho"].as<uint16_t>(), WEB);
  }
  // Learn-Leave mode toggle
  if (root["itho_llm"].is<bool>() && root["itho_llm"].as<bool>())
  {
    toggleRemoteLLmode("remote");
  }

  // copy_id => for virtualRemotes
  if (root["copy_id"].is<JsonObject>())
  {
    auto copyObj = root["copy_id"].as<JsonObject>();
    int number = copyObj["index"] | 0;
    if (number > 0 && number <= virtualRemotes.getMaxRemotes())
    {
      virtualRemotes.copy_id_remote_idx = number - 1;
      toggleRemoteLLmode("vremote");
    }
  }

  // Removing or updating rf remote
  if (root["itho_remove_remote"].is<int>())
  {
    int index = root["itho_remove_remote"].as<int>();
    if (index > 0 && index <= remotes.getMaxRemotes())
    {
      remotes.removeRemote(index - 1);
      saveRemotesflag = true;
    }
  }
  if (root["itho_update_remote"].is<uint8_t>())
  {
    uint8_t idx = root["itho_update_remote"].as<uint8_t>();

    // name
    const char *val = root["value"].is<const char *>() ? root["value"].as<const char *>() : "";
    remotes.updateRemoteName(idx, val);

    // function
    auto remfunc = (RemoteFunctions)(root["remfunc"].is<uint8_t>() ? root["remfunc"].as<uint8_t>() : 0);
    remotes.updateRemoteFunction(idx, remfunc);

    // type
    auto rtype = (RemoteTypes)(root["remtype"].is<uint16_t>() ? root["remtype"].as<uint16_t>() : 0);
    remotes.updateRemoteType(idx, rtype);

    // bidirectional
    bool bidir = root["bidirectional"].is<bool>() ? root["bidirectional"].as<bool>() : false;
    remotes.updateRemoteBidirectional(idx, bidir);

    // id array
    if (root["id"].is<JsonArray>())
    {
      JsonArray arr = root["id"].as<JsonArray>();
      if (arr.size() >= 3)
      {
        uint8_t id0 = arr[0].as<uint8_t>();
        uint8_t id1 = arr[1].as<uint8_t>();
        uint8_t id2 = arr[2].as<uint8_t>();
        remotes.updateRemoteID(idx, id0, id1, id2);
        rf.updateRFDevice(idx, id0, id1, id2, remotes.getRemoteType(idx), bidir);
      }
    }
    saveRemotesflag = true;
  }

  // Removing or updating virtual remote
  if (root["itho_remove_vremote"].is<int>())
  {
    int index = root["itho_remove_vremote"].as<int>();
    if (index > 0 && index <= virtualRemotes.getMaxRemotes())
    {
      virtualRemotes.removeRemote(index - 1);
      saveVremotesflag = true;
    }
  }
  if (root["itho_update_vremote"].is<uint8_t>())
  {
    uint8_t idx = root["itho_update_vremote"].as<uint8_t>();

    const char *val = root["value"].is<const char *>() ? root["value"].as<const char *>() : "";
    virtualRemotes.updateRemoteName(idx, val);

    int vtype = root["remtype"].is<int>() ? root["remtype"].as<int>() : 0;
    virtualRemotes.updateRemoteType(idx, vtype);

    // ID array
    if (root["id"].is<JsonArray>())
    {
      JsonArray arr = root["id"].as<JsonArray>();
      if (arr.size() >= 3)
      {
        virtualRemotes.updateRemoteID(idx,
                                      arr[0].as<uint8_t>(),
                                      arr[1].as<uint8_t>(),
                                      arr[2].as<uint8_t>());
      }
    }
    saveVremotesflag = true;
  }

  // update_rf_id => array of 3 bytes
  if (root["update_rf_id"].is<JsonArray>())
  {
    JsonArray arr = root["update_rf_id"].as<JsonArray>();
    if (arr.size() >= 3)
    {
      uint8_t id0 = arr[0].as<uint8_t>();
      uint8_t id1 = arr[1].as<uint8_t>();
      uint8_t id2 = arr[2].as<uint8_t>();
      if (id0 == 0 && id1 == 0 && id2 == 0)
      {
        systemConfig.module_rf_id[0] = sys.getMac(3);
        systemConfig.module_rf_id[1] = sys.getMac(4);
        systemConfig.module_rf_id[2] = sys.getMac(5) - 1;
      }
      else
      {
        systemConfig.module_rf_id[0] = id0;
        systemConfig.module_rf_id[1] = id1;
        systemConfig.module_rf_id[2] = id2;
      }
      rf.setDefaultID(systemConfig.module_rf_id[0],
                      systemConfig.module_rf_id[1],
                      systemConfig.module_rf_id[2]);
      saveSystemConfigflag = true;
    }
  }

  // Reboot
  if (root["reboot"].is<bool>())
  {
    shouldReboot = root["reboot"].as<bool>();
  }

  // Misc config flags
  if (root["saveallconfigs"].is<bool>() && root["saveallconfigs"].as<bool>())
  {
    saveAllConfigsflag = true;
  }
  if (root["resetwificonf"].is<bool>() && root["resetwificonf"].as<bool>())
  {
    resetWifiConfigflag = true;
  }
  if (root["resetsysconf"].is<bool>() && root["resetsysconf"].as<bool>())
  {
    resetSystemConfigflag = true;
  }
  if (root["format"].is<bool>() && root["format"].as<bool>())
  {
    formatFileSystem = true;
  }

  // RF debug
  if (root["rfdebug"].is<uint16_t>())
  {
    uint16_t val = root["rfdebug"].as<uint16_t>();
    if (val == 0x31DA)
    {
      if (root["faninfo"].is<uint8_t>())
        faninfo31DA = root["faninfo"].as<uint8_t>();
      if (root["timer"].is<uint8_t>())
        timer31DA = root["timer"].as<uint8_t>();
      send31DAdebug = true;
    }
    else if (val == 0x31D9)
    {
      if (root["status"].is<uint8_t>())
      {
        status31D9 = root["status"].as<uint8_t>() * 2;
      }
      if (root["fault"].is<bool>())
        fault31D9 = root["fault"].as<bool>();
      if (root["frost"].is<bool>())
        frost31D9 = root["frost"].as<bool>();
      if (root["filter"].is<bool>())
        filter31D9 = root["filter"].as<bool>();
      send31D9debug = true;
    }
  }

  // I2C sniffer
  if (root["i2csniffer"].is<uint8_t>())
  {
    uint8_t val = root["i2csniffer"].as<uint8_t>();
    if (val == 1)
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
#error "handle_ws_message not implemented for Mongoose WS"
    // handle_ws_message(std::move(msg)); --> handle_ws_message(JsonObject);

    wm = nullptr;
    // msg = std::string();
  }
}
#else
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (!client->id())
    return;

  if (type == WS_EVT_CONNECT)
  {
    client->setCloseClientOnQueueFull(false);
    return;
  }
  else if (type == WS_EVT_DISCONNECT)
  {
    g_wsBuffers[client->id()].clear();
    // Serial.printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id(), client->id());
  }
  else if (type == WS_EVT_ERROR)
  {
    g_wsBuffers[client->id()].clear();
    // Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t *)arg), (char *)data);
  }
  else if (type == WS_EVT_PONG)
  {
    // Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
  }

  else if (type == WS_EVT_DATA)
  {

    AwsFrameInfo *info = (AwsFrameInfo *)arg;

    // info->index == 0 => this is the beginning of a new message
    if (info->index == 0)
    {
      // Clear (or create) the buffer for this client
      g_wsBuffers[client->id()].clear();
    }

    // Append this frame’s data if opcode == WS_TEXT
    if (info->opcode == WS_TEXT)
    {

      auto &msgBuffer = g_wsBuffers[client->id()];
      msgBuffer.append(reinterpret_cast<char *>(data), len);

      size_t currentSize = info->index + len;

      // If this is the final frame, we parse the entire message
      if (currentSize >= info->len)
      {
        JsonDocument wsDoc;

        auto error = deserializeJson(wsDoc, msgBuffer);
        if (error)
        {
          if (msgBuffer == "ping")
          {
            client->text("pong");
          }
          else
          {
            E_LOG("[WS] JSON parse error: %s", error.c_str());
            Serial.println(msgBuffer.c_str());
          }
        }
        // buffer no longer needed
        g_wsBuffers[client->id()].clear();

        // if there was an error, we can now safely return (buffer is cleared)
        if (error)
          return;

        // Convert doc to a JsonObject
        JsonObject wsObj = wsDoc.as<JsonObject>();
        if (wsObj.isNull())
        {
          return;
        }
        handle_ws_message(wsObj);
      }
      else
      {
        // WS_TEXT partial frame handled, return to process the next message
        return;
      }

      // at this point the message is either handled or can be discarded, clear the buffer before return of function
      g_wsBuffers[client->id()].clear();
    }
  }
}
#endif