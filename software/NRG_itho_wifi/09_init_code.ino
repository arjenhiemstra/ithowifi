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

void hardwareInit() {

  Wire.begin(SDAPIN, SCLPIN, 0);

  pinMode(STATUSPIN, INPUT);
  pinMode(WIFILED, OUTPUT);
  digitalWrite(WIFILED, HIGH);

#if defined (__HW_VERSION_TWO__) && defined (ENABLE_FAILSAVE_BOOT)
  pinMode(FAILSAVE_PIN, INPUT);
  failSafeBoot();
#endif

  delay(500); //give itho time to init

  if (digitalRead(STATUSPIN) == LOW) {
    strcpy(i2cstat, "initok");
  }
  else {
    strcpy(i2cstat, "nok");
  }

}

void logInit() {
  filePrint.open();

  Log.begin(LOG_LEVEL_NOTICE, &filePrint);
  Log.setPrefix(printTimestamp);
  Log.setSuffix(printNewline);

  filePrint.close();

  delay(100);
  char logBuff[LOG_BUF_SIZE] = "";


#if defined (__HW_VERSION_ONE__)
  sprintf(logBuff, "System boot, last reset reason: %s", ESP.getResetReason().c_str());
#elif defined (__HW_VERSION_TWO__)
  uint8_t reason = esp_reset_reason();
  char buf[32] = "";

  switch ( reason)
  {
    case 1 : strcpy(buf, "POWERON_RESET"); break;         /**<1,  Vbat power on reset*/
    case 3 : strcpy(buf, "SW_RESET"); break;              /**<3,  Software reset digital core*/
    case 4 : strcpy(buf, "OWDT_RESET"); break;            /**<4,  Legacy watch dog reset digital core*/
    case 5 : strcpy(buf, "DEEPSLEEP_RESET"); break;       /**<5,  Deep Sleep reset digital core*/
    case 6 : strcpy(buf, "SDIO_RESET"); break;            /**<6,  Reset by SLC module, reset digital core*/
    case 7 : strcpy(buf, "TG0WDT_SYS_RESET"); break;      /**<7,  Timer Group0 Watch dog reset digital core*/
    case 8 : strcpy(buf, "TG1WDT_SYS_RESET"); break;      /**<8,  Timer Group1 Watch dog reset digital core*/
    case 9 : strcpy(buf, "RTCWDT_SYS_RESET"); break;      /**<9,  RTC Watch dog Reset digital core*/
    case 10 : strcpy(buf, "INTRUSION_RESET"); break;      /**<10, Instrusion tested to reset CPU*/
    case 11 : strcpy(buf, "TGWDT_CPU_RESET"); break;      /**<11, Time Group reset CPU*/
    case 12 : strcpy(buf, "SW_CPU_RESET"); break;         /**<12, Software reset CPU*/
    case 13 : strcpy(buf, "RTCWDT_CPU_RESET"); break;     /**<13, RTC Watch dog Reset CPU*/
    case 14 : strcpy(buf, "EXT_CPU_RESET"); break;        /**<14, for APP CPU, reseted by PRO CPU*/
    case 15 : strcpy(buf, "RTCWDT_BROWN_OUT_RESET"); break; /**<15, Reset when the vdd voltage is not stable*/
    case 16 : strcpy(buf, "RTCWDT_RTC_RESET"); break;     /**<16, RTC Watch dog reset digital core and rtc module*/
    default : strcpy(buf, "NO_MEAN");
  }
  sprintf(logBuff, "System boot, last reset reason: %s", buf);
#endif

  logInput(logBuff);

}

#if defined(ENABLE_SHT30_SENSOR_SUPPORT)
void initSensor() {

  if (sht_org.init() && sht_org.readSample()) {
    Wire.endTransmission(true);
    SHT3x_original = true;
  }
  else if (sht_alt.init() && sht_alt.readSample()) {
    Wire.endTransmission(true);
    SHT3x_alternative = true;
  }
  if (SHT3x_original) {
    logInput("Setup: Original SHT30 sensor found");
  }
  else if (SHT3x_alternative) {
    logInput("Setup: Alternative SHT30 sensor found");
  }
  else {
    logInput("Setup: SHT30 sensor not present");
  }
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

void wifiInit() {
  if (!loadWifiConfig()) {
    setupWiFiAP();
  }
  else if (!connectWiFiSTA()) {
    setupWiFiAP();
  }
  configTime(0, 0, "pool.ntp.org");

  WiFi.scanDelete();
  if (WiFi.scanComplete() == -2) {
    WiFi.scanNetworks(true);
  }
  if (!wifiModeAP) {
    logWifiInfo();
  }
  else {
    logInput("Setup: AP mode active");
  }

}
void setupWiFiAP() {

  /* Soft AP network parameters */
  IPAddress apIP(192, 168, 4, 1);
  IPAddress netMsk(255, 255, 255, 0);


  WiFi.persistent(false); // Do not use SDK storage of SSID/WPA parameters
#if defined (__HW_VERSION_TWO__)
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
#endif  
  WiFi.disconnect(true);
  WiFi.setAutoReconnect(false);  
  delay(200);
  WiFi.mode(WIFI_AP);

#if defined (__HW_VERSION_ONE__)
  WiFi.forceSleepWake();
#elif defined (__HW_VERSION_TWO__)
  esp_wifi_set_ps(WIFI_PS_NONE);
#endif
  delay(100);
  
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(hostName(), WiFiAPPSK);

  delay(500);

  /* Setup the DNS server redirecting all the domains to the apIP */
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(53, "*", apIP);

  wifiModeAP = true;

  APmodeTimeout = millis();

}


bool connectWiFiSTA()
{
  wifiModeAP = false;

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
  delay(100);

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
    if (!gateway.fromString(wifiConfig.subnet)) {
      configOK = false;
    }
    if (!subnet.fromString(wifiConfig.gateway)) {
      configOK = false;
    }
    if (!dns1.fromString(wifiConfig.dns1)) {
      configOK = false;
    }
    if (!dns2.fromString(wifiConfig.dns2)) {
      configOK = false;
    }
    if (configOK) {
      WiFi.config(staticIP, gateway, subnet, dns1 , dns2);
    }


  }

#if defined (__HW_VERSION_ONE__)
  WiFi.hostname(hostName());
  WiFi.begin(wifiConfig.ssid, wifiConfig.passwd);
#elif defined (__HW_VERSION_TWO__)
  WiFi.setHostname(hostName());
  WiFi.begin(wifiConfig.ssid, wifiConfig.passwd);
#endif

  int i = 0;
#if defined (__HW_VERSION_ONE__)
  while (wifi_station_get_connect_status() != STATION_GOT_IP && i < 16) {
#elif defined (__HW_VERSION_TWO__)
  while ((WiFi.status() != WL_CONNECTED) && i < 16) {
#endif
    delay(2000);
    if (digitalRead(WIFILED) == LOW) {
      digitalWrite(WIFILED, HIGH);
    }
    else {
      digitalWrite(WIFILED, LOW);
    }
    ++i;
  }
#if defined (__HW_VERSION_ONE__)
  if (wifi_station_get_connect_status() != STATION_GOT_IP && i >= 15) {
#elif defined (__HW_VERSION_TWO__)
  if ((WiFi.status() != WL_CONNECTED) && i >= 15) {
#endif
    //delay(1000);
    //Serial.println("");
    //Serial.println("Couldn't connect to network :( ");
    digitalWrite(WIFILED, HIGH);
    return false;

  }

  digitalWrite(WIFILED, LOW);
  return true;

}

void mqttInit() {
  char logBuff[LOG_BUF_SIZE] = "";

  if (strcmp(systemConfig.mqtt_active, "on") == 0) {
    if (!setupMQTTClient()) {
      sprintf(logBuff, "MQTT: connection failed, System config: %s", systemConfig.mqtt_active);
    }
    else {
      sprintf(logBuff, "MQTT: connected, System config: %s", systemConfig.mqtt_active);
    }
    logInput(logBuff);
  }
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

void ArduinoOTAinit() {

  events.onConnect([](AsyncEventSourceClient * client) {
    client->send("hello!", NULL, millis(), 1000);
  });
  server.addHandler(&events);

  // Send OTA events to the browser
  ArduinoOTA.onStart([]() {
    Serial.end();
    events.send("Update Start", "ota");
  });
  ArduinoOTA.onEnd([]() {
    events.send("Update End", "ota");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    char p[32];
    sprintf(p, "Progress: %u%%\n", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    if (error == OTA_AUTH_ERROR)
      events.send("Auth Failed", "ota");
    else if (error == OTA_BEGIN_ERROR)
      events.send("Begin Failed", "ota");
    else if (error == OTA_CONNECT_ERROR)
      events.send("Connect Failed", "ota");
    else if (error == OTA_RECEIVE_ERROR)
      events.send("Recieve Failed", "ota");
    else if (error == OTA_END_ERROR)
      events.send("End Failed", "ota");
  });
  ArduinoOTA.setHostname(hostName());
  ArduinoOTA.begin();

}

void websocketInit() {
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
}

void webServerInit() {

  // respond to GET requests on URL /heap
  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  // upload a file to /upload
  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest * request) {
    request->send(200);
  }, [](AsyncWebServerRequest * request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    //Handle upload
  });


#if defined (__HW_VERSION_ONE__)
  server.addHandler(new SPIFFSEditor(http_username, http_password));
#elif defined (__HW_VERSION_TWO__)
  server.addHandler(new SPIFFSEditor(SPIFFS, http_username, http_password));
#endif

  // css_code
  server.on("/pure-min.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css", pure_min_css_gz, pure_min_css_gz_len);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  // javascript files
  server.on("/js/zepto.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "application/javascript", zepto_min_js_gz, zepto_min_js_gz_len);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.on("/js/controls.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "application/javascript", controls_js);
    response->addHeader("Server", "Itho WiFi Web Server");
    request->send(response);
  });

  server.on("/js/general.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncResponseStream *response = request->beginResponseStream("application/javascript");
    response->addHeader("Server", "Itho WiFi Web Server");

    response->print("var on_ap = false; var hostname = '");
    response->print(hostName());
    response->print("'; var fw_version = '");
    response->print(FWVERSION);
    response->print("'; var hw_revision = '");
    response->print(HWREVISION);
    response->print("'; $(document).ready(function() { $('#headingindex').text(hostname); $('#headingindex').attr('href', 'http://' + hostname + '.local'); $('#main').append(html_index); });");

    request->send(response);
  }).setFilter(ON_STA_FILTER);

  server.on("/js/general.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    AsyncResponseStream *response = request->beginResponseStream("application/javascript");
    response->addHeader("Server", "Itho WiFi Web Server");

    response->print("var on_ap = true; var hostname = '");
    response->print(hostName());
    response->print("'; var fw_version = '");
    response->print(FWVERSION);
    response->print("'; var hw_revision = '");
    response->print(HWREVISION);
    response->print("'; $(document).ready(function() { $('#headingindex').text(hostname); $('#headingindex').attr('href', 'http://' + hostname + '.local'); $('#main').append(html_wifisetup); });");

    request->send(response);
  }).setFilter(ON_AP_FILTER);

  // HTML pages
  server.rewrite("/", "/index.htm");
  server.on("/index.htm", HTTP_ANY, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", html_mainpage);
  });
  server.on("/api.html", HTTP_GET, handleAPI);
  server.on("/debug", HTTP_GET, handleDebug);

  //Log file download
  server.on("/curlog", HTTP_GET, handleCurLogDownload);
  server.on("/prevlog", HTTP_GET, handlePrevLogDownload);

  // HTTP basic authentication
  server.on("/login", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (!request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    request->send(200, "text/plain", "Login Success!");
  });
  // Simple Firmware Update Form
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/html", "<form method='POST' action='/update' "
                  "enctype='multipart/form-data'><input "
                  "type='file' name='update'><input "
                  "type='submit' value='Update'></form>");
  });

  server.on("/update", HTTP_POST, [](AsyncWebServerRequest * request) {
    shouldReboot = !Update.hasError();
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", shouldReboot ? "OK" : "FAIL");
    response->addHeader("Connection", "close");
    request->send(response);
  },
  [](AsyncWebServerRequest * request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!index) {
      dontSaveConfig = true;
      dontReconnectMQTT = true;
      mqttClient.disconnect();

      content_len = request->contentLength();
      static char buf[128] = "";
      strcat(buf, "Firmware update: ");
      strncat(buf, filename.c_str(), sizeof(buf) - strlen(buf) - 1);
      logInput(buf);

#if defined (__HW_VERSION_TWO__)

      detachInterrupt(ITHO_IRQ_PIN);

      TaskCC1101Timeout.detach();
      TaskConfigAndLogTimeout.detach();
      TaskMQTTTimeout.detach();

      vTaskSuspend( xTaskCC1101Handle );
      vTaskSuspend( xTaskMQTTHandle );
      vTaskSuspend( xTaskConfigAndLogHandle );

      esp_task_wdt_delete( xTaskCC1101Handle );
      esp_task_wdt_delete( xTaskMQTTHandle );
      esp_task_wdt_delete( xTaskConfigAndLogHandle );

#endif

#if defined (__HW_VERSION_ONE__)
      Update.runAsync(true);
      if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) {
#elif defined (__HW_VERSION_TWO__)
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
#endif
        Update.printError(Serial);
      } else {
        Serial.end();
      }
    }
    if (!Update.hasError()) {
      if (Update.write(data, len) != len) {
        Update.printError(Serial);
      }
    }
    if (final) {
      if (Update.end(true)) {
        // Serial.printf("Update Success: %uB\n", index+len);
      } else {
        Update.printError(Serial);
      }
    }
  });
  Update.onProgress(otaWSupdate);

  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest * request) {
    jsonLogMessage(F("Reset requested Device will reboot in a few seconds..."), WEBINTERFACE);
    delay(200);
    shouldReboot = true;
  });

  // Catch-All Handlers
  // Any request that can not find a Handler that canHandle it
  // ends in the callbacks below.
  server.onNotFound([](AsyncWebServerRequest * request) {
    //Handle Unknown Request
    request->send(404);
  });


  server.begin();

  logInput("Webserver: started");
}

void MDNSinit() {


#if defined (__HW_VERSION_TWO__)
  MDNS.begin(hostName());
  mdns_instance_name_set("NRG IthoWifi controller");
#endif
  MDNS.addService("http", "tcp", 80);

  char logBuff[LOG_BUF_SIZE] = "";
  logInput("mDNS: started");

  sprintf(logBuff, "Hostname: %s", hostName());
  logInput(logBuff);


}
