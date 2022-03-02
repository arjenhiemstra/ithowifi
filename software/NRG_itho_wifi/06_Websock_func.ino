

void jsonWsSend(const char* rootName) {
  DynamicJsonDocument root(4000);

  if (strcmp(rootName, "wifisettings") == 0) {
    JsonObject nested = root.createNestedObject(rootName);
    nested["ssid"] = wifiConfig.ssid;
    nested["passwd"] = wifiConfig.passwd;
    nested["dhcp"] = wifiConfig.dhcp;
    nested["renew"] = wifiConfig.renew;
    if (strcmp(wifiConfig.dhcp, "off") == 0) {
      nested["ip"] = wifiConfig.ip;
      nested["subnet"] = wifiConfig.subnet;
      nested["gateway"] = wifiConfig.gateway;
      nested["dns1"] = wifiConfig.dns1;
      nested["dns2"] = wifiConfig.dns2;
    }
    else {
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
  else if (strcmp(rootName, "systemsettings")  == 0) {
    // Create an object at the root
    JsonObject nested = root.createNestedObject(rootName);
    systemConfig.get(nested);
  }
  else if (strcmp(rootName, "ithodevinfo")  == 0) {
    // Create an object at the root
    JsonObject nested = root.createNestedObject(rootName);
    nested["itho_devtype"] = getIthoType(ithoDeviceID);
    nested["itho_fwversion"] = itho_fwversion;
    nested["itho_setlen"] = ithoSettingsLength;
  }
  else if (strcmp(rootName, "ithosatusinfo")  == 0) {
    // Create an object at the root
    JsonObject nested = root.createNestedObject(rootName);
    getIthoStatusJSON(nested);
  }
  else if (strcmp(rootName, "remotes") == 0) {
    // Create an object at the root
    JsonObject obj = root.to<JsonObject>(); // Fill the object
    remotes.get(obj, rootName);
  }
  else if (strcmp(rootName, "vremotes") == 0) {
    // Create an object at the root
    JsonObject obj = root.to<JsonObject>(); // Fill the object
    virtualRemotes.get(obj, rootName);
  }
  notifyClients(root.as<JsonObjectConst>());
}

void jsonSystemstat() {
  StaticJsonDocument<512> root;
  //JsonObject root = jsonBuffer.createObject();
  JsonObject systemstat = root.createNestedObject("systemstat");
  systemstat["freemem"] = sys.getMemHigh();
  systemstat["memlow"] = sys.getMemLow();
  systemstat["mqqtstatus"] = MQTT_conn_state;
  systemstat["itho"] = ithoCurrentVal;
  systemstat["itho_low"] = systemConfig.itho_low;
  systemstat["itho_medium"] = systemConfig.itho_medium;
  systemstat["itho_high"] = systemConfig.itho_high;

  if (SHT3x_original || SHT3x_alternative || itho_internal_hum_temp)
  {
    systemstat["sensor_temp"] = ithoTemp;
    systemstat["sensor_hum"] = ithoHum;
    systemstat["sensor"] = 1;
  }
  systemstat["itho_llm"] = remotes.getllModeTime();
  systemstat["copy_id"] = virtualRemotes.getllModeTime();
  systemstat["ithoinit"] = ithoInitResult;

  notifyClients(root.as<JsonObjectConst>());

}

static void wsEvent(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {
  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *) ev_data;
    if (mg_http_match_uri(hm, "/ws")) {
      // Upgrade to websocket. From now on, a connection is a full-duplex
      // Websocket connection, which will receive MG_EV_WS_MSG events.
      mg_ws_upgrade(c, hm, NULL);
    }
  }
  else if (ev == MG_EV_WS_MSG) {
    // Got websocket frame. Received data is wm->data.
    struct mg_ws_message *wm = (struct mg_ws_message *) ev_data;
    std::string msg = wm->data.ptr;

    D_LOG("%s\n", msg.c_str());

    if (msg.find("{\"wifiscan\"") != std::string::npos) {
      D_LOG("Start wifi scan\n");
      runscan = true;
    }
    if (msg.find("{\"sysstat\"") != std::string::npos) {
      sysStatReq = true;
    }
    else if (msg.find("{\"wifisettings\"") != std::string::npos || msg.find("{\"systemsettings\"") != std::string::npos) {
      DynamicJsonDocument root(2048);
      DeserializationError error = deserializeJson(root, msg.c_str());
      if (!error) {
        JsonObject obj = root.as<JsonObject>();
        for (JsonPair p : obj) {
          if (strcmp(p.key().c_str(), "systemsettings") == 0) {
            if (p.value().is<JsonObject>()) {

              JsonObject obj = p.value();
              if (systemConfig.set(obj)) {
                saveSystemConfigflag = true;
              }
            }
          }
          if (strcmp(p.key().c_str(), "wifisettings") == 0) {
            if (p.value().is<JsonObject>()) {

              JsonObject obj = p.value();
              if (wifiConfig.set(obj)) {
                saveWifiConfigflag = true;
              }
            }
          }
        }
      }
    }
    else if (msg.find("{\"ithobutton\"") != std::string::npos) {
      StaticJsonDocument<128> root;
      DeserializationError error = deserializeJson(root, msg);
      if (!error) {
        if (root["ithobutton"].as<const char*>() == nullptr) {
          uint16_t val  = root["ithobutton"].as<uint16_t>();
          if (val == 2410) {
            getSetting(root["index"].as<int8_t>(), false, true, false);
          }
          else if (val == 24109) {
            if (ithoSettingsArray != nullptr) {
              uint8_t index = root["ithosetupdate"].as<uint8_t>();
              if (ithoSettingsArray[index].type == ithoSettings::is_float2) {
                updateSetting(root["ithosetupdate"].as<uint8_t>(), (int32_t)(root["value"].as<float>() * 2), false);
              }
              else if (ithoSettingsArray[index].type == ithoSettings::is_float10) {
                updateSetting(root["ithosetupdate"].as<uint8_t>(), (int32_t)(root["value"].as<float>() * 10), false);
              }
              else if (ithoSettingsArray[index].type == ithoSettings::is_float100) {
                updateSetting(root["ithosetupdate"].as<uint8_t>(), (int32_t)(root["value"].as<float>() * 100), false);
              }
              else if (ithoSettingsArray[index].type == ithoSettings::is_float1000) {
                updateSetting(root["ithosetupdate"].as<uint8_t>(), (int32_t)(root["value"].as<float>() * 1000), false);
              }
              else {
                updateSetting(root["ithosetupdate"].as<uint8_t>(), root["value"].as<int32_t>(), false);
              }
            }
          }
        }
        else {
          i2c_result_updateweb = true;
          ithoI2CCommand(root["ithobutton"], WEB);
        }
      }
    }
    else if (msg.find("{\"wifisetup\"") != std::string::npos) {
      jsonWsSend("wifisettings");
    }
    else if (msg.find("{\"syssetup\"") != std::string::npos) {
      systemConfig.get_sys_settings = true;
      jsonWsSend("systemsettings");
    }
    else if (msg.find("{\"mqttsetup\"") != std::string::npos) {
      systemConfig.get_mqtt_settings = true;
      jsonWsSend("systemsettings");
      sysStatReq = true;
    }
    else if (msg.find("{\"ithosetup\"") != std::string::npos) {
      jsonWsSend("ithodevinfo");
      sysStatReq = true;
    }
    else if (msg.find("{\"ithostatus\"") != std::string::npos) {
      jsonWsSend("ithosatusinfo");
      sysStatReq = true;
    }
    else if (msg.find("{\"ithogetsetting\"") != std::string::npos) {
      StaticJsonDocument<128> root;
      DeserializationError error = deserializeJson(root, msg.c_str());
      if (!error) {
        getSetting(root["index"].as<uint8_t>(), root["update"].as<bool>(), false, true);
      }
    }
    else if (msg.find("{\"ithosetrefresh\"") != std::string::npos) {
      StaticJsonDocument<128> root;
      DeserializationError error = deserializeJson(root, msg.c_str());
      if (!error) {
        getSetting(root["ithosetrefresh"].as<uint8_t>(), true, false, false);
      }
    }
    else if (msg.find("{\"ithosetupdate\"") != std::string::npos) {
      StaticJsonDocument<128> root;
      DeserializationError error = deserializeJson(root, msg.c_str());
      if (!error) {
        if (ithoSettingsArray != nullptr) {
          uint8_t index = root["ithosetupdate"].as<uint8_t>();
          if (ithoSettingsArray[index].type == ithoSettings::is_float2) {
            updateSetting(root["ithosetupdate"].as<uint8_t>(), (int32_t)(root["value"].as<float>() * 2), false);
          }
          else if (ithoSettingsArray[index].type == ithoSettings::is_float10) {
            updateSetting(root["ithosetupdate"].as<uint8_t>(), (int32_t)(root["value"].as<float>() * 10), false);
          }
          else if (ithoSettingsArray[index].type == ithoSettings::is_float100) {
            updateSetting(root["ithosetupdate"].as<uint8_t>(), (int32_t)(root["value"].as<float>() * 100), false);
          }
          else if (ithoSettingsArray[index].type == ithoSettings::is_float1000) {
            updateSetting(root["ithosetupdate"].as<uint8_t>(), (int32_t)(root["value"].as<float>() * 1000), false);
          }
          else {
            updateSetting(root["ithosetupdate"].as<uint8_t>(), root["value"].as<int32_t>(), false);
          }
        }
      }
    }
    else if (msg.find("{\"ithoremotes\"") != std::string::npos) {
      jsonWsSend("remotes");
      sysStatReq = true;
    }
    else if (msg.find("{\"ithovremotes\"") != std::string::npos) {
      jsonWsSend("vremotes");
      sysStatReq = true;
    }
    else if (msg.find("{\"vremote\"") != std::string::npos) {
      StaticJsonDocument<128> root;
      DeserializationError error = deserializeJson(root, msg.c_str());
      if (!error) {
        if (!(const char*)root["vremote"].isNull() && !(const char*)root["command"].isNull()) {
          ithoI2CButtonCommand(root["vremote"].as<uint8_t>(), root["command"].as<const char*>());
        }
      }

    }
    else if (msg.find("{\"command\"") != std::string::npos) {
      StaticJsonDocument<128> root;
      DeserializationError error = deserializeJson(root, msg.c_str());
      if (!error) {
        if (!(const char*)root["command"].isNull()) {
          ithoExecCommand( root["command"].as<const char*>(), WEB);
        }
      }

    }    

    
    else if (msg.find("{\"itho_llm\"") != std::string::npos) {
      toggleRemoteLLmode("remote");
    }
    else if (msg.find("{\"copy_id\"") != std::string::npos) {
      bool parseOK = false;
      StaticJsonDocument<128> root;
      DeserializationError error = deserializeJson(root, msg.c_str());
      if (!error) {
        int number = root["index"];
        if (number > 0 && number < virtualRemotes.getMaxRemotes() + 1) {
          number--; //+1 was added earlier to offset index from 0
          parseOK = true;
        }
        if (parseOK) {
          virtualRemotes.activeRemote = number;
          toggleRemoteLLmode("vremote");
        }
      }

    }
    else if (msg.find("{\"itho_ccc_toggle\"") != std::string::npos) {
      toggleCCcal();
    }
    if (msg.find("{\"itho_ccc_reset\"") != std::string::npos) {
      resetCCcal();
      ithoCCstatReq = true;
    }
    if (msg.find("{\"ithoccc\"") != std::string::npos) {
      ithoCCstatReq = true;
    }
    else if (msg.find("{\"itho_remove_remote\"") != std::string::npos) {
      bool parseOK = false;
      StaticJsonDocument<128> root;
      DeserializationError error = deserializeJson(root, msg.c_str());
      if (!error) {
        int number = root["itho_remove_remote"];
        if (number > 0 && number < remotes.getMaxRemotes() + 1) {
          parseOK = true;
          number--; //+1 was added earlier to offset index from 0
        }
        if (parseOK) {
          remotes.removeRemote(number, "remote");
          const int* id = remotes.getRemoteIDbyIndex(number);
          rf.setBindAllowed(true);
          rf.removeRFDevice(*id, *(id + 1), *(id + 2));
          rf.setBindAllowed(false);
          saveRemotesflag = true;
        }
      }
    }
    else if (msg.find("{\"itho_update_remote\"") != std::string::npos) {
      StaticJsonDocument<512> root;
      DeserializationError error = deserializeJson(root, msg.c_str());
      if (!error) {
        uint8_t index = root["itho_update_remote"].as<unsigned int>();
        remotes.updateRemoteName(index, root["value"] | "");
        remotes.updateRemoteType(index, root["remtype"] | 0);
        saveRemotesflag = true;
      }
    }
    else if (msg.find("{\"itho_remove_vremote\"") != std::string::npos) {
      bool parseOK = false;
      StaticJsonDocument<128> root;
      DeserializationError error = deserializeJson(root, msg.c_str());
      if (!error) {
        int number = root["itho_remove_vremote"];
        if (number > 0 && number < virtualRemotes.getMaxRemotes() + 1) {
          parseOK = true;
          number--; //+1 was added earlier to offset index from 0
        }
        if (parseOK) {
          virtualRemotes.removeRemote(number, "vremote");
          saveVremotesflag = true;
        }
      }
    }
    else if (msg.find("{\"itho_update_vremote\"") != std::string::npos) {
      StaticJsonDocument<512> root;
      DeserializationError error = deserializeJson(root, msg.c_str());
      if (!error) {
        uint8_t index = root["itho_update_vremote"].as<unsigned int>();
        virtualRemotes.updateRemoteName(index, root["value"] | "");
        virtualRemotes.updateRemoteType(index, root["remtype"] | 0);
        saveVremotesflag = true;
      }
    }
    else if (msg.find("{\"reboot\"") != std::string::npos) {
      StaticJsonDocument<128> root;
      DeserializationError error = deserializeJson(root, msg.c_str());
      if (!error) {
        shouldReboot = root["reboot"];
        dontSaveConfig = root["dontsaveconf"];
      }
    }
    else if (msg.find("{\"resetwificonf\"") != std::string::npos) {
      resetWifiConfigflag = true;
    }
    else if (msg.find("{\"resetsysconf\"") != std::string::npos) {
      resetSystemConfigflag = true;
    }
    else if (msg.find("{\"format\"") != std::string::npos) {
      formatFileSystem = true;
    }
    else if (msg.find("{\"itho\"") != std::string::npos) {
      StaticJsonDocument<128> root;
      DeserializationError error = deserializeJson(root, msg);
      if (!error) {
        ithoSetSpeed(root["itho"].as<uint16_t>(), WEB);
      }
    }
    else if (msg.find("{\"rfdebug\"") != std::string::npos) {
      StaticJsonDocument<128> root;
      DeserializationError error = deserializeJson(root, msg.c_str());
      if (!error) {
        setRFdebugLevel(root["rfdebug"].as<uint8_t>());
      }
    }

  }
  (void) fn_data;
}


void wifiScan() {

  int n = WiFi.scanComplete();
  if (n == -2) {
    WiFi.scanNetworks(true);
  }
  else if (n) {
    char logBuff[LOG_BUF_SIZE] = "";
    sprintf(logBuff, "Wifi scan found %d networks", n);
    logMessagejson(logBuff, WEBINTERFACE);

    //sort networks
    int indices[n];
    for (int i = 0; i < n; i++) {
      indices[i] = i;
    }

    for (int i = 0; i < n; i++) {
      for (int j = i + 1; j < n; j++) {
        if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) {
          std::swap(indices[i], indices[j]);
        }
      }
    }

    //send scan result to DOM
    for (int i = 0; i < n; i++) {

      int sec = 0;
      switch (WiFi.encryptionType(indices[i])) {
        case WIFI_AUTH_OPEN:
          sec = 1; //open network
          break;
        case WIFI_AUTH_WEP:
        case WIFI_AUTH_WPA_PSK:
        case WIFI_AUTH_WPA2_PSK:
        case WIFI_AUTH_WPA_WPA2_PSK:
        case WIFI_AUTH_WPA2_ENTERPRISE:
        case WIFI_AUTH_MAX:
          sec = 2; //closed network
          break;
        default:
          sec = 0; //unknown network encryption
      }


      int32_t signalStrength = WiFi.RSSI(indices[i]);
      uint8_t signalStrengthResult = 0;

      if (signalStrength > -65) {
        signalStrengthResult = 5;
      }
      else if (signalStrength > -75) {
        signalStrengthResult = 4;
      }
      else if (signalStrength > -80) {
        signalStrengthResult = 3;
      }
      else if (signalStrength > -90) {
        signalStrengthResult = 2;
      }
      else if (signalStrength > -100) {
        signalStrengthResult = 1;
      }
      else {
        signalStrengthResult = 0;
      }

      char ssid[33]; //ssid can be up to 32chars, => plus null term
      strcpy(ssid, WiFi.SSID(indices[i]).c_str());

      StaticJsonDocument<512> root;
      JsonObject wifiscanresult = root.createNestedObject("wifiscanresult");
      wifiscanresult["id"] = i;
      wifiscanresult["ssid"] = ssid;
      wifiscanresult["sigval"] = signalStrengthResult;
      wifiscanresult["sec"] = sec;

      notifyClients(root.as<JsonObjectConst>());

      delay(25);
    }
    WiFi.scanDelete();
    if (WiFi.scanComplete() == -2) {
      WiFi.scanNetworks(true);
    }
  }

}
