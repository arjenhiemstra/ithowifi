

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
    nested[F("port")] = wifiConfig.port;

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
    if (!ithoStatus.empty()) {
      for (const auto& ithoStat : ithoStatus) {
        if (ithoStat.type == ihtoDeviceStatus::is_byte) {
          nested[ithoStat.name.c_str()] = ithoStat.value.byteval;
        }
        else if (ithoStat.type == ihtoDeviceStatus::is_uint) {
          nested[ithoStat.name.c_str()] = ithoStat.value.uintval;
        }
        else if (ithoStat.type == ihtoDeviceStatus::is_int) {
          nested[ithoStat.name.c_str()] = ithoStat.value.intval;
        }
        else if (ithoStat.type == ihtoDeviceStatus::is_float) {
          nested[ithoStat.name.c_str()] = ithoStat.value.floatval;
        }
        else {
          nested["error"] = 0;
        }
      }
    }
  }

#if defined (HW_VERSION_TWO)
  else if (strcmp(rootName, "ithoremotes") == 0) {
    // Create an object at the root
    JsonObject obj = root.to<JsonObject>(); // Fill the object
    remotes.get(obj);
  }
#endif
  size_t len = measureJson(root);
  AsyncWebSocketMessageBuffer * buffer = ws.makeBuffer(len); //  creates a buffer (len + 1) for you.
  if (buffer) {
    serializeJson(root, (char *)buffer->get(), len + 1);
    notifyClients(buffer);
  }
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
#if defined (HW_VERSION_TWO)
  systemstat["itho_llm"] = remotes.getllModeTime();
#endif
  systemstat["ithoinit"] = ithoInitResult;

  char buffer[512];
  size_t len = serializeJson(root, buffer);

  notifyClients(buffer, len);
}


void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    //Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
    client->ping();
  } else if (type == WS_EVT_DISCONNECT) {
    //Serial.printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id(), client->id());
  } else if (type == WS_EVT_ERROR) {
    //Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t*)arg), (char*)data);
  } else if (type == WS_EVT_PONG) {
    //Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len)?(char*)data:"");
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo * info = (AwsFrameInfo*)arg;
    std::string msg;
    if (info->final && info->index == 0 && info->len == len) {
      //the whole message is in a single frame and we got all of it's data
      //Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", info->len);

      if (info->opcode == WS_TEXT) {
        msg.reserve(len + 2);
        for (size_t i = 0; i < info->len; i++) {
          msg += (char) data[i];
        }
      } else {
        msg.reserve(len * 3 + 2);
        char buff[3];
        for (size_t i = 0; i < info->len; i++) {
          sprintf(buff, "%02x ", (uint8_t) data[i]);
          msg += buff ;
        }
      }
      //Serial.printf("%s\n",msg.c_str());

      if (msg.find("{\"wifiscan\"")!=std::string::npos) {
        //Serial.println("Start wifi scan");
        runscan = true;
      }
      if (msg.find("{\"sysstat\"")!=std::string::npos) {
        sysStatReq = true;
      }
      else if (msg.find("{\"wifisettings\"")!=std::string::npos || msg.find("{\"systemsettings\"")!=std::string::npos) {
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
      else if (msg.find("{\"ithobutton\"")!=std::string::npos) {
        StaticJsonDocument<128> root;
        DeserializationError error = deserializeJson(root, msg);
        if (!error) {
          uint16_t val  = root["ithobutton"];
          if (val < 4) {
            buttonValue = val;
            sendI2CButton = true;
            i2c_result_updateweb = true;
          }
          else if (val == 11) {
            sendI2CJoin = true;
            i2c_result_updateweb = true;
          }
          else if (val == 20) {
            sendI2CDevicetype = true;
            i2c_result_updateweb = true;
          }
          else if (val == 30) {
            sendI2CStatus = true;
            i2c_result_updateweb = true;
          }
          else if (val == 31) {
            sendI2CStatusFormat = true;
            i2c_result_updateweb = true;
          }
          else if (val == 2400) {
            send2400 = true;
            i2c_result_updateweb = true;
          }
          else if (val == 2401) {
            send2401 = true;
            i2c_result_updateweb = true;
          }
          else if (val == 2410) {
            index2410  = root["index"];
            get2410 = true;
            i2c_result_updateweb = true;
          }
          else if (val == 24109) {
            index2410 = root["index"];
            value2410 = root["value"];
            set2410 = true;
            i2c_result_updateweb = true;
          }
          else if (val == 12762) {
            send31DA = true;
            i2c_result_updateweb = true;
          }
          else if (val == 12761) {
            send31D9 = true;
            i2c_result_updateweb = true;
          }
          else if (val == 99) {
            sendI2CLeave = true;
            i2c_result_updateweb = true;
          }
        }
      }
      else if (msg.find("{\"wifisetup\"")!=std::string::npos) {
        jsonWsSend("wifisettings");
      }
      else if (msg.find("{\"syssetup\"")!=std::string::npos) {
        systemConfig.get_sys_settings = true;
        jsonWsSend("systemsettings");
      }
      else if (msg.find("{\"mqttsetup\"")!=std::string::npos) {
        systemConfig.get_mqtt_settings = true;
        jsonWsSend("systemsettings");
        sysStatReq = true;
      }
      else if (msg.find("{\"ithosetup\"")!=std::string::npos) {
        jsonWsSend("ithodevinfo");
        sysStatReq = true;
      }
      else if (msg.find("{\"ithostatus\"")!=std::string::npos) {
        jsonWsSend("ithosatusinfo");
        sysStatReq = true;
      }
      else if (msg.find("{\"ithogetsetting\"")!=std::string::npos) {
        StaticJsonDocument<128> root;
        DeserializationError error = deserializeJson(root, msg.c_str());
        if (!error) {
          uint8_t index = root["index"].as<unsigned int>();
          bool updateState = root["update"].as<bool>();
          getSetting(index, updateState, true);
        }
      }
      else if (msg.find("{\"ithosetrefresh\"")!=std::string::npos) {
        StaticJsonDocument<128> root;
        DeserializationError error = deserializeJson(root, msg.c_str());
        if (!error) {
          uint8_t index = root["ithosetrefresh"].as<unsigned int>();
          getSetting(index, true, false);
        }
      }
      else if (msg.find("{\"ithosetupdate\"")!=std::string::npos) {
        StaticJsonDocument<128> root;
        DeserializationError error = deserializeJson(root, msg.c_str());
        if (!error) {
          uint8_t index = root["ithosetupdate"].as<unsigned int>();
          int32_t value = root["value"].as<int32_t>();
          updateSetting(index, value);
        }
      }

#if defined (HW_VERSION_TWO)
      else if (msg.find("{\"ithoremotes\"")!=std::string::npos) {
        jsonWsSend("ithoremotes");
        sysStatReq = true;
      }
      else if (msg.find("{\"itho_llm\"")!=std::string::npos) {
        toggleRemoteLLmode();
      }
      else if (msg.find("{\"itho_ccc_toggle\"")!=std::string::npos) {
        toggleCCcal();
      }
      if (msg.find("{\"itho_ccc_reset\"")!=std::string::npos) {
        resetCCcal();
        ithoCCstatReq = true;
      }
      if (msg.find("{\"ithoccc\"")!=std::string::npos) {
        ithoCCstatReq = true;
      }
      else if (msg.find("{\"itho_remove_remote\"")!=std::string::npos) {
        bool parseOK = false;
        int number = atoi( (msg.substr(22,len)).c_str() );
        if (number == 0) {
          if (strcmp(msg.substr(22, 23).c_str(), "0") == 0) {
            parseOK = true;
          }
        }
        else if (number > 0 && number < MAX_NUMBER_OF_REMOTES) {
          parseOK = true;
        }
        if (parseOK) {
          remotes.removeRemote(remotes.getRemoteIDbyIndex(number));
          saveRemotesflag = true;
        }
      }
      else if (msg.find("{\"itho_update_remote\"")!=std::string::npos) {
        StaticJsonDocument<512> root;
        DeserializationError error = deserializeJson(root, msg.c_str());
        if (!error) {
          uint8_t index = root["itho_update_remote"].as<unsigned int>();
          char remoteName[32];
          strlcpy(remoteName, root["value"] | "", sizeof(remoteName));
          remotes.updateRemoteName(index, remoteName);
          saveRemotesflag = true;
        }
      }
#endif
      else if (msg.find("{\"reboot\"")!=std::string::npos) {
        shouldReboot = true;
      }
      else if (msg.find("{\"resetwificonf\"")!=std::string::npos) {
        resetWifiConfigflag = true;
      }
      else if (msg.find("{\"resetsysconf\"")!=std::string::npos) {
        resetSystemConfigflag = true;
      }
      else if (msg.find("{\"format\"")!=std::string::npos) {
        formatFileSystem = true;
      }
      else if (msg.find("{\"itho\"")!=std::string::npos) {
        StaticJsonDocument<128> root;
        DeserializationError error = deserializeJson(root, msg);
        if (!error) {
          nextIthoVal  = root["itho"];
          nextIthoTimer = 0;
          updateItho = true;
        }
      }
    }
  }
}



void wifiScan() {

  int n = WiFi.scanComplete();
  if (n == -2) {
    WiFi.scanNetworks(true);
  }
  else if (n) {
    char logBuff[LOG_BUF_SIZE] = "";
    sprintf(logBuff, "Wifi scan found %d networks", n);
    jsonLogMessage(logBuff, WEBINTERFACE);

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

      char buffer[512];
      size_t len = serializeJson(root, buffer);

      notifyClients(buffer, len);

      delay(25);
    }
    WiFi.scanDelete();
    if (WiFi.scanComplete() == -2) {
      WiFi.scanNetworks(true);
    }
  }

}
