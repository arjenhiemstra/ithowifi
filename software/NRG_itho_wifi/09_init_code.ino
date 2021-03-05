#if defined (__HW_VERSION_TWO__) && defined (ENABLE_FAILSAVE_BOOT)
void failSafeBoot() {

  long defaultWaitStart = millis();
  long ledblink = 0;

  while (millis() - defaultWaitStart < 2000) {
    yield();

    if (digitalRead(FAILSAVE_PIN) == HIGH) {

      initFileSystem();
      resetWifiConfig();
      resetSystemConfig();

      IPAddress apIP(192, 168, 4, 1);
      IPAddress netMsk(255, 255, 255, 0);

      WiFi.persistent(false);
      WiFi.disconnect(); //  this alone is not enough to stop the autoconnecter
      WiFi.mode(WIFI_AP);
      delay(2000);

      WiFi.softAP(hostName(), WiFiAPPSK);
      WiFi.softAPConfig(apIP, apIP, netMsk);


      delay(500);

      /* Setup the DNS server redirecting all the domains to the apIP */
      dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
      dnsServer.start(53, "*", apIP);

      // Simple Firmware Update Form
      server.on("/update", HTTP_GET, [](AsyncWebServerRequest * request) {
        request->send(200, "text/html", "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>");
      });


      server.on("/update", HTTP_POST, [](AsyncWebServerRequest * request) {
        shouldReboot = !Update.hasError();
        AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", shouldReboot ? "OK" : "FAIL");
        response->addHeader("Connection", "close");
        request->send(response);
      }, [](AsyncWebServerRequest * request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
        if (!index) {
          content_len = request->contentLength();
          //Serial.printf("Update Start: %s\n", filename.c_str());
          if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
          }

        }
        if (!Update.hasError()) {
          if (Update.write(data, len) != len) {

            Update.printError(Serial);
          }
        }
        if (final) {
          if (Update.end(true)) {
          } else {
            Update.printError(Serial);
          }
          shouldReboot = true;
        }
      });
      server.begin();

      for (;;) {
        yield();
        if (shouldReboot) {
          delay(1000);
          esp_task_wdt_init(1, true);
          esp_task_wdt_add(NULL);
          while (true);
        }
      }

    }


    if (millis() - ledblink > 50) {
      ledblink = millis();
      if (digitalRead(WIFILED) == LOW) {
        digitalWrite(WIFILED, HIGH);
      }
      else {
        digitalWrite(WIFILED, LOW);
      }
    }

  }
  digitalWrite(WIFILED, HIGH);
}

#endif

bool initFileSystem() {

  //Serial.println("Mounting FS...");

#if defined (__HW_VERSION_ONE__)

  if (!SPIFFS.begin()) {
    //Serial.println("SPIFFS failed, needs formatting");
    handleFormat();
    delay(500);
    ESP.restart();
  }
  else {
    if (!SPIFFS.info(fs_info)) {
      //Serial.println("fs_info failed");
      return false;
    }
  }

#elif defined (__HW_VERSION_TWO__)
  SPIFFS.begin(true);
#endif

  return true;
}


void handleFormat()
{
  //Serial.println("Format SPIFFS");
  if (SPIFFS.format())
  {
    if (!SPIFFS.begin())
    {
      //Serial.println("Format SPIFFS failed");
    }
  }
  else
  {
    //Serial.println("Format SPIFFS failed");
  }
  if (!SPIFFS.begin())
  {
    //Serial.println("SPIFFS failed, needs formatting");
  }
  else
  {
    //Serial.println("SPIFFS mounted");
  }
}


char* hostName() {
  static char hostName[32];
  // Do a little work to get a unique-ish name. Append the
  // last two bytes of the MAC (HEX'd):
  uint8_t mac[6];
#if defined (__HW_VERSION_ONE__)
  WiFi.softAPmacAddress(mac);
#elif defined (__HW_VERSION_TWO__)
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
#endif

  sprintf(hostName, "%s%02x%02x", espName, mac[6 - 2], mac[6 - 1]);

  return hostName;
}

void setupWiFiAP() {

  /* Soft AP network parameters */
  IPAddress apIP(192, 168, 4, 1);
  IPAddress netMsk(255, 255, 255, 0);

  WiFi.disconnect(true);
  delay(30);
  WiFi.mode(WIFI_OFF);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(false);

#if defined (__HW_VERSION_ONE__)
  WiFi.forceSleepWake();
#elif defined (__HW_VERSION_TWO__)
  esp_wifi_set_ps(WIFI_PS_NONE);
#endif

  delay(100);
  WiFi.mode(WIFI_AP);

  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(hostName(), WiFiAPPSK);

  delay(500);

  /* Setup the DNS server redirecting all the domains to the apIP */
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(53, "*", apIP);

  wifiModeAP = true;

  APmodeTimeout = millis();

}


bool connectWiFiSTA() {
   wifiModeAP = false;
#if defined (INFORMATIVE_LOGGING)
  logInput("Connecting to wireless network...");
#endif
  WiFi.persistent(false); // Do not use SDK storage of SSID/WPA parameters
#if defined (__HW_VERSION_TWO__)
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
#endif
  WiFi.disconnect(true);
  WiFi.setAutoReconnect(false);
  delay(200);
  WiFi.mode(WIFI_STA);

#if defined (__HW_VERSION_ONE__)
  WiFi.forceSleepWake();
#elif defined (__HW_VERSION_TWO__)
  esp_wifi_set_ps(WIFI_PS_NONE);
#endif

  if (strcmp(wifiConfig.dhcp, "off") == 0) {
    bool configOK = true;
    IPAddress staticIP;
    IPAddress gateway;
    IPAddress subnet;
    IPAddress dns1;
    IPAddress dns2;

    if (!staticIP.fromString(wifiConfig.ip)) {
      configOK = false;
    }
    if (!gateway.fromString(wifiConfig.gateway)) {
      configOK = false;
    }
    if (!subnet.fromString(wifiConfig.subnet)) {
      configOK = false;
    }
    if (!dns1.fromString(wifiConfig.dns1)) {
      configOK = false;
    }
    if (!dns2.fromString(wifiConfig.dns2)) {
      configOK = false;
    }
    if (configOK) {
      if(WiFi.config(staticIP, gateway, subnet, dns1 , dns2)) {
        logInput("Static IP config OK");
      }
    }
    else {
      logInput("Static IP config NOK");
    }
  }

  delay(200);

#if defined (__HW_VERSION_ONE__)
  WiFi.hostname(hostName());
  WiFi.begin(wifiConfig.ssid, wifiConfig.passwd);
#elif defined (__HW_VERSION_TWO__)
  WiFi.setHostname(hostName());
  WiFi.begin(wifiConfig.ssid, wifiConfig.passwd);
#endif

  unsigned long timeoutmillis = millis() + 30000;
  uint8_t status = WiFi.status();

  while (millis() < timeoutmillis) {
#if defined (__HW_VERSION_TWO__)    
    esp_task_wdt_reset();
#endif
    status = WiFi.status();
    if (status == WL_CONNECTED) {
      digitalWrite(WIFILED, LOW);
      return true;
    }
    if (digitalRead(WIFILED) == LOW) {
      digitalWrite(WIFILED, HIGH);
    }
    else {
      digitalWrite(WIFILED, LOW);
    }

    delay(100);
  }
  digitalWrite(WIFILED, HIGH);
  return false;

}

bool setupMQTTClient() {
  int connectResult;

  if (strcmp(systemConfig.mqtt_active, "on") == 0) {

    if (systemConfig.mqtt_serverName != "") {

      mqttClient.setServer(systemConfig.mqtt_serverName, systemConfig.mqtt_port);
      mqttClient.setCallback(mqttCallback);
      mqttClient.setBufferSize(1024);

      if (systemConfig.mqtt_username == "") {
        connectResult = mqttClient.connect(hostName());
      }
      else {
        connectResult = mqttClient.connect(hostName(), systemConfig.mqtt_username, systemConfig.mqtt_password);
      }

      if (!connectResult) {
        return false;
      }

      if (mqttClient.connected()) {
        if (mqttClient.subscribe(systemConfig.mqtt_cmd_topic)) {
          //subscribed succes
        }
        else {
          //subscribed failed
        }
        if (mqttClient.subscribe(systemConfig.mqtt_state_topic)) {
          //publish succes
        }
        else {
          //publish failed
        }
        return true;
      }
    }

  }
  else {
    mqttClient.disconnect();
    return false;
  }

}

boolean reconnect() {
  setupMQTTClient();
  return mqttClient.connected();
}

void logWifiInfo() {

  char wifiBuff[128];

  logInput("WiFi: connection successful");


  logInput("WiFi info:");

  const char* const modes[] = { "NULL", "STA", "AP", "STA+AP" };
  const char* const phymodes[] = { "", "B", "G", "N" };

#if defined (__HW_VERSION_ONE__)
  sprintf(wifiBuff, "Mode:%s", modes[wifi_get_opmode()]);
  logInput(wifiBuff);
  strcpy(wifiBuff, "");

  sprintf(wifiBuff, "PHY mode:%s", phymodes[(int) wifi_get_phy_mode()]);
  logInput(wifiBuff);
  strcpy(wifiBuff, "");

  sprintf(wifiBuff, "Channel:%d", wifi_get_channel());
  logInput(wifiBuff);
  strcpy(wifiBuff, "");

  sprintf(wifiBuff, "AP id:%d", wifi_station_get_current_ap_id());
  logInput(wifiBuff);
  strcpy(wifiBuff, "");

  sprintf(wifiBuff, "Status:%d", wifi_station_get_connect_status());
  logInput(wifiBuff);
  strcpy(wifiBuff, "");

  sprintf(wifiBuff, "Auto connect:%d", wifi_station_get_auto_connect());
  logInput(wifiBuff);
  strcpy(wifiBuff, "");

  struct station_config conf;
  wifi_station_get_config(&conf);

  char ssid[33]; //ssid can be up to 32chars, => plus null term
  memcpy(ssid, conf.ssid, sizeof(conf.ssid));
  ssid[32] = 0; //nullterm in case of 32 char ssid

  sprintf(wifiBuff, "SSID (%d):%s", strlen(ssid), ssid);
  logInput(wifiBuff);
  strcpy(wifiBuff, "");

#elif defined (__HW_VERSION_TWO__)

  sprintf(wifiBuff, "Mode:%s", modes[WiFi.getMode()]);
  logInput(wifiBuff);
  strcpy(wifiBuff, "");

  sprintf(wifiBuff, "Status:%d", WiFi.status());
  logInput(wifiBuff);
  strcpy(wifiBuff, "");
#endif

  IPAddress ip = WiFi.localIP();
  sprintf(wifiBuff, "IP:%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  logInput(wifiBuff);
  strcpy(wifiBuff, "");


}
