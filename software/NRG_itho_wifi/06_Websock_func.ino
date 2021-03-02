


void notifyClients(AsyncWebSocketMessageBuffer* message) {
#if defined (__HW_VERSION_TWO__)
  yield();
  if (xSemaphoreTake(mutexWSsend, (TickType_t) 100 / portTICK_PERIOD_MS) == pdTRUE) {
#endif

    ws.textAll(message);

#if defined (__HW_VERSION_TWO__)
    xSemaphoreGive(mutexWSsend);
  }
#endif

}

void notifyClients(const char * message, size_t len) {

  AsyncWebSocketMessageBuffer * WSBuffer = ws.makeBuffer((uint8_t *)message, len);
  notifyClients(WSBuffer);

}

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
#if defined (__HW_VERSION_TWO__)
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


void jsonLogMessage(const __FlashStringHelper * str, logtype type) {
  if (!str) return;
  int length = strlen_P((PGM_P)str);
  if (length == 0) return;
#if defined (__HW_VERSION_ONE__)
  if (length < 400) length = 400;
  char message[400 + 1] = "";
  strncat_P(message, (PGM_P)str, length);
  jsonLogMessage(message, type);
#else
  jsonLogMessage((PGM_P)str, type);
#endif
}

void jsonLogMessage(const char* message, logtype type) {
#if defined (__HW_VERSION_TWO__)
  yield();
  if (xSemaphoreTake(mutexJSONLog, (TickType_t) 500 / portTICK_PERIOD_MS) == pdTRUE) {
#endif

    StaticJsonDocument<512> root;
    JsonObject messagebox;

    switch (type) {
      case RFLOG:
        messagebox = root.createNestedObject("rflog");
        break;
      default:
        messagebox = root.createNestedObject("messagebox");
    }

    messagebox["message"] = message;

    char buffer[512];
    size_t len = serializeJson(root, buffer);


    notifyClients(buffer, len);

#if defined (__HW_VERSION_TWO__)
    xSemaphoreGive(mutexJSONLog);
  }
#endif




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
#if defined (__HW_VERSION_TWO__)
  systemstat["itho_llm"] = remotes.getllModeTime();
#endif
  systemstat["i2cstat"] = i2cstat;

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
    String msg = "";
    if (info->final && info->index == 0 && info->len == len) {
      //the whole message is in a single frame and we got all of it's data
      //Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", info->len);

      if (info->opcode == WS_TEXT) {
        for (size_t i = 0; i < info->len; i++) {
          msg += (char) data[i];
        }
      } else {
        char buff[3];
        for (size_t i = 0; i < info->len; i++) {
          sprintf(buff, "%02x ", (uint8_t) data[i]);
          msg += buff ;
        }
      }
      //Serial.printf("%s\n",msg.c_str());

      if (msg.startsWith("{\"wifiscan")) {
        //Serial.println("Start wifi scan");
        runscan = true;
      }
      if (msg.startsWith("{\"sysstat")) {
        sysStatReq = true;
      }
      else if (msg.startsWith("{\"wifisettings") || msg.startsWith("{\"systemsettings")) {
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
      else if (msg.startsWith("{\"wifisetup")) {
        jsonWsSend("wifisettings");
      }
      else if (msg.startsWith("{\"syssetup")) {
        systemConfig.get_sys_settings = true;
        jsonWsSend("systemsettings");
      }
      else if (msg.startsWith("{\"mqttsetup")) {
        systemConfig.get_mqtt_settings = true;
        jsonWsSend("systemsettings");
        sysStatReq = true;
      }
      else if (msg.startsWith("{\"ithosetup")) {
        systemConfig.get_itho_settings = true;
        jsonWsSend("systemsettings");
        sysStatReq = true;
      }
#if defined (__HW_VERSION_TWO__)
      else if (msg.startsWith("{\"ithoremotes")) {
        jsonWsSend("ithoremotes");
      }
      else if (msg.startsWith("{\"itho_llm")) {
        toggleRemoteLLmode();
      }
      else if (msg.startsWith("{\"itho_remove_remote")) {
        bool parseOK = false;
        int number = (msg.substring(22)).toInt();
        if (number == 0) {
          if (strcmp(msg.substring(22, 23).c_str(), "0") == 0) {
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
      else if (msg.startsWith("{\"itho_update_remote")) {
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
      else if (msg.startsWith("{\"reboot")) {
        shouldReboot = true;
      }
      else if (msg.startsWith("{\"resetwificonf")) {
        resetWifiConfigflag = true;
      }
      else if (msg.startsWith("{\"resetsysconf")) {
        resetSystemConfigflag = true;
      }
      else if (msg.startsWith("{\"itho")) {
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
        case 2:
        case 4:
        case 5:
          sec = 2; //closed network
          break;
        case 7:
          sec = 1; //open network
          break;
        case 8:
          sec = 2;
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
      String ssidStr = WiFi.SSID(indices[i]);
      ssidStr.toCharArray(ssid, sizeof(ssid));

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


unsigned long LastotaWsUpdate = 0;

void otaWSupdate(size_t prg, size_t sz) {
  
  if (millis() - LastotaWsUpdate >= 500) { //rate limit messages to once a second
    LastotaWsUpdate = millis();
    int newPercent = int((prg * 100) / content_len);

    StaticJsonDocument<256> root;
    JsonObject ota = root.createNestedObject("ota");
    ota["progress"] = prg;
    ota["tsize"] = content_len;
    ota["percent"] = newPercent;

    char buffer[256];
    size_t len = serializeJson(root, buffer);

    notifyClients(buffer, len);

  }


}
