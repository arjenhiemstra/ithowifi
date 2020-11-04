


void jsonWsSend(const char* rootName) {
  DynamicJsonDocument root(4000);

  if (strcmp(rootName, "wifisettings") == 0) {
    JsonObject nested = root.createNestedObject(rootName);
    nested["ssid"] = wifiConfig.ssid;
    nested["passwd"] = wifiConfig.passwd;
    nested["dhcp"] = wifiConfig.dhcp;
    nested["renew"] = wifiConfig.renew;
    if (nested["dhcp"] == "off") {
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
  else if (strcmp(rootName, "mqttsettings")  == 0) {
    // Create an object at the root
    JsonObject nested = root.createNestedObject(rootName);
    systemConfig.get(nested);
  }
  else if (strcmp(rootName, "ithosettings") == 0) {
    // Create an object at the root
    JsonObject obj = root.to<JsonObject>(); // Fill the object
    remotes.get(obj);
  }
  size_t len = measureJson(root);
  AsyncWebSocketMessageBuffer * buffer = ws.makeBuffer(len); //  creates a buffer (len + 1) for you.
  if (buffer) {
    serializeJson(root, (char *)buffer->get(), len + 1);
    ws.textAll(buffer);
  }
}

// Convert & Transfer Arduino elements to JSON elements
void jsonWifiscanresult(int id, const char* ssid, int sigval, int sec) {
  StaticJsonDocument<512> root;
  //JsonObject root = jsonBuffer.createObject();
  JsonObject wifiscanresult = root.createNestedObject("wifiscanresult");
  wifiscanresult["id"] = id;
  wifiscanresult["ssid"] = ssid;
  wifiscanresult["sigval"] = sigval;
  wifiscanresult["sec"] = sec;

  char buffer[512];
  size_t len = serializeJson(root, buffer);

  ws.textAll(buffer, len);

}
// Convert & Transfer Arduino elements to JSON elements
void jsonMessageBox(char* message1, char* message2) {
  StaticJsonDocument<128> root;
  //JsonObject root = jsonBuffer.createObject();
  JsonObject messagebox = root.createNestedObject("messagebox");
  messagebox["message1"] = message1;
  messagebox["message2"] = message2;

  char buffer[128];
  size_t len = serializeJson(root, buffer);

  ws.textAll(buffer, len);
}
// Convert & Transfer Arduino elements to JSON elements
void jsonSystemstat() {
  StaticJsonDocument<256> root;
  //JsonObject root = jsonBuffer.createObject();
  JsonObject systemstat = root.createNestedObject("systemstat");
  systemstat["freemem"] = sys.getMemHigh();
  systemstat["memlow"] = sys.getMemLow();
  systemstat["mqqtstatus"] = MQTT_conn_state;
  systemstat["itho"] = itho_current_val;
  systemstat["i2cstat"] = i2cstat;

  char buffer[256];
  size_t len = serializeJson(root, buffer);

  ws.textAll(buffer, len);
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
      Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT)?"text":"binary", info->len);

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
      else if (msg.startsWith("{\"wifisettings") || msg.startsWith("{\"wifisettings")) {
        DynamicJsonDocument root(2048);
        DeserializationError error = deserializeJson(root, msg);
        if (!error) {
          JsonObject obj = root.as<JsonObject>();
          for (JsonPair p : obj) {
            if (strcmp(p.key().c_str(), "mqttsettings") == 0) {
              Serial.println("mqttsettings key match");

              if (p.value().is<JsonObject>()) {

                JsonObject obj = p.value();
                if (systemConfig.set(obj)) {
                  if (saveSystemConfig()) {
                    jsonMessageBox("System settings saved", "successful");
                  }
                  else {
                    jsonMessageBox("System settings save failed:", "Unable to write config file");
                  }
                  mqttSetup = true;
                }
              }
            }
            if (strcmp(p.key().c_str(), "wifisettings") == 0) {
              Serial.println("wifisettings key match");

              if (p.value().is<JsonObject>()) {

                JsonObject obj = p.value();
                if (wifiConfig.set(obj)) {
                  if (saveWifiConfig()) {
                    jsonMessageBox("Wifi settings saved successful,", "reboot the device");
                  }
                  else {
                    jsonMessageBox("Wifi settings save failed:", "Unable to write config file");
                  }
                }
              }
            }
          }
        }

      }
      else if (msg.startsWith("{\"wifisetup")) {
        jsonWsSend("wifisettings");
      }
      else if (msg.startsWith("{\"mqttsetup")) {
        jsonWsSend("mqttsettings");
        sysStatReq = true;
      }
      else if (msg.startsWith("{\"ithosettings")) {
        jsonWsSend("ithosettings");
      }
      else if (msg.startsWith("{\"reboot")) {
        shouldReboot = true;
      }
      else if (msg.startsWith("{\"resetwificonf")) {
        if (resetWifiConfig()) {
          jsonMessageBox("Wifi settings restored,", "reboot the device");
        }
        else {
          jsonMessageBox("Wifi settings restore failed,", "please try again");
        }
      }
      else if (msg.startsWith("{\"resetsysconf")) {
        if (resetSystemConfig()) {
          jsonMessageBox("System settings restored,", "reboot the device");
        }
        else {
          jsonMessageBox("System settings restore failed,", "please try again");
        }
      }
      else if (msg.startsWith("{\"itho")) {
        StaticJsonDocument<128> root;
        DeserializationError error = deserializeJson(root, msg);
        if (!error) {
          itho_new_val  = root["itho"];
          updateItho = true;
        }
      }
    }
  }
}





void sendScanDataWs()
{
  int n = WiFi.scanComplete();
  if (n == -2) {
    WiFi.scanNetworks(true);
  }
  else if (n) {
    sprintf(logBuff, "Wifi scan found %d networks", n);
    logInput(logBuff);
    jsonMessageBox(logBuff, "");
    strcpy(logBuff, "");
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

      sprintf(logBuff, "%d - SSID:%s, Signal:%d, Security: %d", i, ssid, signalStrengthResult, sec);
      logInput(logBuff);
      strcpy(logBuff, "");

      jsonWifiscanresult(i, WiFi.SSID(indices[i]).c_str(), signalStrengthResult, sec);
    }
    WiFi.scanDelete();
    if (WiFi.scanComplete() == -2) {
      WiFi.scanNetworks(true);
    }
  }

}


int LastPercentotaWSupdate = 0;

void otaWSupdate(size_t prg, size_t sz) {
  int newPercent = int((prg * 100) / content_len);
  if (newPercent != LastPercentotaWSupdate) {
    LastPercentotaWSupdate = newPercent;
    if (newPercent % 2 == 0) {
      StaticJsonDocument<256> root;
      JsonObject ota = root.createNestedObject("ota");
      ota["progress"] = prg;
      ota["tsize"] = content_len;
      ota["percent"] = newPercent;

      char buffer[256];
      size_t len = serializeJson(root, buffer);

      ws.textAll(buffer, len);
    }
  }

}
