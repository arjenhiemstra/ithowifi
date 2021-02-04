void setup() {
  Wire.begin(SDAPIN, SCLPIN, 0);
    
  pinMode(STATUSPIN, INPUT);
  pinMode(WIFILED, OUTPUT);
  digitalWrite(WIFILED, HIGH);

#if defined (__HW_VERSION_TWO__) && defined (ENABLE_FAILSAVE_BOOT)
  pinMode(FAILSAVE_PIN, INPUT);
  failSafeBoot();
#endif
  
//  Serial.begin(115200);
//  Serial.flush();

  if (!initFileSystem()) {
    //Serial.println("\nFile system erroor :( ");
  }
  else {
    //Serial.println("\nFile system OK");

    filePrint.open();

    Log.begin(LOG_LEVEL_NOTICE, &filePrint);
    Log.setPrefix(printTimestamp);
    Log.setSuffix(printNewline);

    filePrint.close();
  }

  if (!loadWifiConfig()) {
    setupWiFiAP();
  }
  else if (!connectWiFiSTA()) {
    setupWiFiAP();
  }
  if (!loadSystemConfig()) {
    //Serial.println("System config error :( ");
  }
  ithoQueue.set_itho_fallback_speed(systemConfig.itho_fallback);
  
  configTime(0, 0, "pool.ntp.org");

#if defined (__HW_VERSION_ONE__)
  sprintf(logBuff, "System boot, last reset reason: %s", ESP.getResetReason().c_str());
#elif defined (__HW_VERSION_TWO__)
  sprintf(logBuff, "System boot, last reset reason: %d", esp_reset_reason());
#endif
  logInput(logBuff);
  strcpy(logBuff, "");
  if (!wifiModeAP) {
    logWifiInfo();
  }
  else {
    logInput("Setup: AP mode active");
  }


  if (strcmp(systemConfig.mqtt_active, "on") == 0) {
    if (!setupMQTTClient()) {
      sprintf(logBuff, "MQTT: connection failed, System config: %s", systemConfig.mqtt_active);
    }
    else {
      sprintf(logBuff, "MQTT: connected, System config: %s", systemConfig.mqtt_active);
    }
    logInput(logBuff);
    strcpy(logBuff, "");
  }

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

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

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
  [](AsyncWebServerRequest * request, String filename, size_t index,
     uint8_t *data, size_t len, bool final) {
    if (!index) {
      content_len = request->contentLength();
#if defined (__HW_VERSION_ONE__)
      Update.runAsync(true);
      if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) {
#elif defined (__HW_VERSION_TWO__)
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
#endif
        Update.printError(Serial);
      } else {
        
#if defined (__HW_VERSION_TWO__)
        detachInterrupt(ITHO_IRQ_PIN);
#endif        
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

#if defined (__HW_VERSION_TWO__)
  MDNS.begin(hostName());
  mdns_instance_name_set("NRG IthoWifi controller");
#endif
  MDNS.addService("http", "tcp", 80);

  logInput("mDNS: started");
  sprintf(logBuff, "Hostname: %s", hostName());
  logInput(logBuff);
  strcpy(logBuff, "");

  WiFi.scanDelete();
  if (WiFi.scanComplete() == -2) {
    WiFi.scanNetworks(true);
  }
  
#if defined (__HW_VERSION_TWO__)
  xTaskCreate(
    CC1101Task,          /* Task function. */
    "CC1101Task",        /* String with name of task. */
    4096,            /* Stack size in bytes. */
    NULL,             /* Parameter passed as input of the task */
    5,                /* Priority of the task. */
    &CC1101TaskHandle);            /* Task handle. */
#endif

  
  
  strcat(i2cstat, "sOk");
  logInput("Setup: done");
}
