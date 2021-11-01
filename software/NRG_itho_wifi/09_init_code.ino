#define MQTT_BUFFER_SIZE 5120
#if defined (HW_VERSION_TWO) && defined (ENABLE_FAILSAVE_BOOT)

void failSafeBoot() {

  auto defaultWaitStart = millis() + 2000;
  auto ledblink = 0;

  while (millis() < defaultWaitStart) {
    yield();

    if (digitalRead(FAILSAVE_PIN) == HIGH) {

      SPIFFS.format();

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
          D_LOG("Update Start: %s\n", filename.c_str());
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
        if (millis() - ledblink > 200) {
          ledblink = millis();
          if (digitalRead(WIFILED) == LOW) {
            digitalWrite(WIFILED, HIGH);
          }
          else {
            digitalWrite(WIFILED, LOW);
          }
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

  //Workaround for https://github.com/arjenhiemstra/ithowifi/issues/30
#if defined (HW_VERSION_TWO) && defined (CVE)
  if (digitalRead(BOOTSTATE) == LOW) {
    pinMode(BOOTSTATE, OUTPUT);
    digitalWrite(BOOTSTATE, HIGH);
    esp_task_wdt_init(2, true);
    esp_task_wdt_add(NULL);
    while (true);
  }
#endif

  pinMode(STATUSPIN, INPUT_PULLUP);
  pinMode(WIFILED, OUTPUT);
  digitalWrite(WIFILED, HIGH);
#if defined (CVE)
  pinMode(ITHOSTATUS, OUTPUT);
  digitalWrite(ITHOSTATUS, LOW);
#endif

#if defined (HW_VERSION_TWO) && defined (ENABLE_FAILSAVE_BOOT)
  pinMode(FAILSAVE_PIN, INPUT);
  failSafeBoot();
#endif

  i2cInit();

  IthoInit = true;
}

void i2cInit() {
#if defined (HW_VERSION_ONE)
  Wire.begin(SDAPIN, SCLPIN, 0);
  //Wire.onReceive(receiveEvent);
#elif defined (HW_VERSION_TWO)
  i2c_master_init();
#endif
}

bool ithoInitCheck() {
#if defined (CVE)  
  if (digitalRead(STATUSPIN) == LOW) {
    return false;
  }
  sendI2CPWMinit();
#endif  
  return false;
}

void logInit() {
  filePrint.open();

  Log.begin(LOG_LEVEL_NOTICE, &filePrint);
  Log.setPrefix(printTimestamp);
  Log.setSuffix(printNewline);

  filePrint.close();

  delay(100);
  char logBuff[LOG_BUF_SIZE] = "";


#if defined (HW_VERSION_ONE)
  sprintf(logBuff, "System boot, last reset reason: %s", ESP.getResetReason().c_str());
#elif defined (HW_VERSION_TWO)
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

  strcpy(logBuff, "");
  sprintf(logBuff, "HW rev: %s, FW ver.: %s", HWREVISION, FWVERSION);
  logInput(logBuff);

}

void initSensor() {

  if (systemConfig.syssht30) {
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
    if (SHT3x_original || SHT3x_alternative) return;

    delay(200);

    if (sht_org.init() && sht_org.readSample()) {
      Wire.endTransmission(true);
      SHT3x_original = true;
    }
    else if (sht_alt.init() && sht_alt.readSample()) {
      Wire.endTransmission(true);
      SHT3x_alternative = true;
    }
    if (SHT3x_original) {
      logInput("Setup: Original SHT30 sensor found (2nd try)");
    }
    else if (SHT3x_alternative) {
      logInput("Setup: Alternative SHT30 sensor found (2nd try)");
    }
    else {
      systemConfig.syssht30 = 0;
      logInput("Setup: SHT30 sensor not present");
    }
  }

}

bool initFileSystem() {

  D_LOG("Mounting FS...\n");

#if defined (HW_VERSION_ONE)

  if (!SPIFFS.begin()) {
    D_LOG("SPIFFS failed, needs formatting\n");
    handleFormat();
    delay(500);
    ESP.restart();
  }
  else {
    if (!SPIFFS.info(fs_info)) {
      D_LOG("fs_info failed\n");
      return false;
    }
  }

#elif defined (HW_VERSION_TWO)
  SPIFFS.begin(true);
#endif

  return true;
}


void handleFormat()
{
  D_LOG("Format SPIFFS\n");
  if (SPIFFS.format())
  {
    if (!SPIFFS.begin())
    {
      D_LOG("Format SPIFFS failed\n");
    }
  }
  else
  {
    D_LOG("Format SPIFFS failed\n");
  }
  if (!SPIFFS.begin())
  {
    D_LOG("SPIFFS failed, needs formatting\n");
  }
  else
  {
    D_LOG("SPIFFS mounted\n");
  }
}


char* hostName() {
  static char hostName[32];

  if (strcmp(wifiConfig.hostname, "") == 0) {
    sprintf(hostName, "%s%02x%02x", espName, getMac(6 - 2), getMac(6 - 1));
  }
  else {
    strlcpy(hostName, wifiConfig.hostname, sizeof(hostName));
  }


  return hostName;
}

uint8_t getMac(uint8_t i) {
  static uint8_t mac[6];

#if defined (HW_VERSION_ONE)
  WiFi.softAPmacAddress(mac);
#elif defined (HW_VERSION_TWO)
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
#endif

  return mac[i];
}

void wifiInit() {
  if (!loadWifiConfig()) {
    logInput("Setup: Wifi config load failed");
    setupWiFiAP();
  }
  else if (!connectWiFiSTA()) {
    logInput("Setup: Wifi connect STA failed");
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
#if defined (HW_VERSION_TWO)
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
#endif
  WiFi.disconnect(true);
  WiFi.setAutoReconnect(false);
  delay(200);
  WiFi.mode(WIFI_AP);

#if defined (HW_VERSION_ONE)
  WiFi.forceSleepWake();
#elif defined (HW_VERSION_TWO)
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
  logInput("wifi AP mode started");

}


bool connectWiFiSTA()
{
  wifiModeAP = false;
  D_LOG("Connecting to wireless network...\n");
  
  WiFi.persistent(false); // Do not use SDK storage of SSID/WPA parameters
#if defined (HW_VERSION_TWO)
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
#endif
  WiFi.disconnect(true);
#if defined (HW_VERSION_TWO)  
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.setHostname(hostName());  
#endif  
  WiFi.setAutoReconnect(false);
  delay(200);
  WiFi.mode(WIFI_STA);

#if defined (HW_VERSION_ONE)
  WiFi.forceSleepWake();
#elif defined (HW_VERSION_TWO)
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
      if (!WiFi.config(staticIP, gateway, subnet, dns1 , dns2)) {
        logInput("Static IP config NOK");
      }
      else {
        logInput("Static IP config OK");
      }
    }
  }

  delay(200);



#if defined (HW_VERSION_ONE)
  WiFi.hostname(hostName());
  WiFi.begin(wifiConfig.ssid, wifiConfig.passwd);
#elif defined (HW_VERSION_TWO)
  WiFi.begin(wifiConfig.ssid, wifiConfig.passwd);
#endif

  auto timeoutmillis = millis() + 30000;
  auto status = WiFi.status();

  while (millis() < timeoutmillis) {
#if defined (HW_VERSION_TWO)
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

void mqttInit() {
  char logBuff[LOG_BUF_SIZE] = "";

  if (systemConfig.mqtt_active) {
    if (!setupMQTTClient()) {
      sprintf(logBuff, "MQTT: connection failed, System config: %d", systemConfig.mqtt_active);
    }
    else {
      sprintf(logBuff, "MQTT: connected, System config: %d", systemConfig.mqtt_active);
    }
    logInput(logBuff);
  }
}


void init_vRemote() {
  //setup virtual remote
  id0 = getMac(6 - 3);
  id1 = getMac(6 - 2);
  id2 = getMac(6 - 1);
  char buff[128];
  sprintf(buff, "Setup: Virtual remote ID: %d,%d,%d", id0, id1, id2);
  logInput(buff);
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

  // upload a file to /upload
  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest * request) {
    if (systemConfig.syssec_web) {
      if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
        return request->requestAuthentication();
    }
    request->send(200);
  }, [](AsyncWebServerRequest * request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    //Handle upload
  });

#if defined (HW_VERSION_ONE)
  if (systemConfig.syssec_edit) {
    server.addHandler(new SPIFFSEditor(systemConfig.sys_username, systemConfig.sys_password));
  }
  else {
    server.addHandler(new SPIFFSEditor("", ""));
  }
#elif defined (HW_VERSION_TWO)
  if (systemConfig.syssec_edit) {
    server.addHandler(new SPIFFSEditor(SPIFFS, systemConfig.sys_username, systemConfig.sys_password));
  }
  else {
    server.addHandler(new SPIFFSEditor(SPIFFS, "", ""));
  }
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
    if (systemConfig.syssec_web) {
      if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
        return request->requestAuthentication();
    }
    request->send_P(200, "text/html", html_mainpage);
  });
  server.on("/api.html", HTTP_GET, handleAPI);
  server.on("/debug", HTTP_GET, handleDebug);

  //Log file download
  server.on("/curlog", HTTP_GET, handleCurLogDownload);
  server.on("/prevlog", HTTP_GET, handlePrevLogDownload);

  // HTTP basic authentication
  server.on("/login", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (systemConfig.syssec_web) {
      if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
        return request->requestAuthentication();
    }
    request->send(200, "text/plain", "Login Success!");
  });
  // Simple Firmware Update Form
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (systemConfig.syssec_web) {
      if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
        return request->requestAuthentication();
    }
    request->send(200, "text/html", "<form method='POST' action='/update' "
                  "enctype='multipart/form-data'><input "
                  "type='file' name='update'><input "
                  "type='submit' value='Update'></form>");
  });

  server.on("/update", HTTP_POST, [](AsyncWebServerRequest * request) {
    if (systemConfig.syssec_web) {
      if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
        return request->requestAuthentication();
    }
    shouldReboot = !Update.hasError();
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", shouldReboot ? "OK" : "FAIL");
    response->addHeader("Connection", "close");
    request->send(response);
  },
  [](AsyncWebServerRequest * request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!index) {
#if defined (ENABLE_SERIAL)
      D_LOG("Begin OTA update\n");
#endif
      onOTA = true;
      dontSaveConfig = true;
      dontReconnectMQTT = true;
      mqttClient.disconnect();

      content_len = request->contentLength();
      static char buf[128] = "";
      strcat(buf, "Firmware update: ");
      strncat(buf, filename.c_str(), sizeof(buf) - strlen(buf) - 1);
      logInput(buf);

#if defined (HW_VERSION_TWO)

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

#if defined (ENABLE_SERIAL)
      D_LOG("Tasks detached\n");
#endif

#endif

#if defined (HW_VERSION_ONE)
      Update.runAsync(true);
      if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) {
#elif defined (HW_VERSION_TWO)
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
#endif
#if defined (ENABLE_SERIAL)
        Update.printError(Serial);
#endif
      }
    }
    if (!Update.hasError()) {
      if (Update.write(data, len) != len) {
#if defined (ENABLE_SERIAL)
        Update.printError(Serial);
#endif
      }
    }
    if (final) {
      if (Update.end(true)) {
#if defined (ENABLE_SERIAL)
        D_LOG("Update Success: %uB\n", index + len);
#endif
      } else {
#if defined (ENABLE_SERIAL)
        Update.printError(Serial);
#endif
      }
      onOTA = false;
    }
  });
  Update.onProgress(otaWSupdate);

  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (systemConfig.syssec_web) {
      if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
        return request->requestAuthentication();
    }
    logMessagejson(F("Reset requested. Device will reboot in a few seconds..."), WEBINTERFACE);
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


#if defined (HW_VERSION_TWO)
  MDNS.begin(hostName());
  mdns_instance_name_set("NRG IthoWifi controller");
#endif
  MDNS.addService("http", "tcp", 80);

  char logBuff[LOG_BUF_SIZE] = "";
  logInput("mDNS: started");

  sprintf(logBuff, "Hostname: %s", hostName());
  logInput(logBuff);


}
