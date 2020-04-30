void setup() {
  Wire.begin(SDAPIN, SCLPIN, 0);

  pinMode(STATUSPIN, INPUT);
  pinMode(WIFILED, OUTPUT);
  digitalWrite(WIFILED, HIGH);
  
//    Serial.begin(115200);
//    Serial.flush();


  delay(1000);

  if (!initFileSystem()) {
    // Serial.println("File system erroor :( ");
  } else if (!loadWifiConfig()) {
    setupWiFiAP();
  } else if (!connectWiFiSTA()) {
    setupWiFiAP();
  }
  if (!loadSystemConfig()) {
    // Serial.println("System config error :( ");
  }
  if (strcmp(systemConfig.mqtt_active, "on") == 0) {
    if (!setupMQTTClient()) {
      // MQTT not connected
    }
  }

  events.onConnect([](AsyncEventSourceClient *client) {
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
  ArduinoOTA.setHostname(EspHostname());
  ArduinoOTA.begin();

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  // respond to GET requests on URL /heap
  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  // upload a file to /upload
  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request) {
    request->send(200);
  }, onUpload);

#if defined(ESP8266)
  server.addHandler(new SPIFFSEditor(http_username, http_password));
#else
  server.addHandler(new SPIFFSEditor(SPIFFS, http_username, http_password));
#endif

  // css_code
  server.on("/pure-min.css", HTTP_GET, css_code);

  // javascript files
  server.on("/js/zepto.min.js", HTTP_GET, zepto_min_js_gz_code);
  server.on("/js/controls.js", HTTP_GET, controls_js_code);
  server.on("/js/general.js", HTTP_GET, handleGeneralJs)
      .setFilter(ON_STA_FILTER);
  server.on("/js/general.js", HTTP_GET, handleGeneralJsOnAp)
      .setFilter(ON_AP_FILTER);

  // HTML pages
  server.rewrite("/", "/index.htm");
  server.on("/index.htm", HTTP_ANY, handleMainpage);
  server.on("/debug", HTTP_GET, handleDebug);

  // HTTP basic authentication
  server.on("/login", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    request->send(200, "text/plain", "Login Success!");
  });
  // Simple Firmware Update Form
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", "<form method='POST' action='/update' "
                                    "enctype='multipart/form-data'><input "
                                    "type='file' name='update'><input "
                                    "type='submit' value='Update'></form>");
  });

  server.on("/update", HTTP_POST,[](AsyncWebServerRequest *request) {
        shouldReboot = !Update.hasError();
        AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", shouldReboot ? "OK" : "FAIL");
        response->addHeader("Connection", "close");
        request->send(response);
      },
      [](AsyncWebServerRequest *request, String filename, size_t index,
         uint8_t *data, size_t len, bool final) {
        if (!index) {
          content_len = request->contentLength();
          #if defined(ESP8266)
          Update.runAsync(true);
          if (!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)) {
          #else
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

  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request) {
    jsonMessageBox("Reset requested ",
                   "Device will reboot in a few seconds...");
    delay(200);
    shouldReboot = true;
  });

  // Catch-All Handlers
  // Any request that can not find a Handler that canHandle it
  // ends in the callbacks below.
  server.onNotFound(onRequest);
  server.onFileUpload(onUpload);
  server.onRequestBody(onBody);

  server.begin();

  MDNS.addService("http", "tcp", 80);

  WiFi.scanDelete();
  if (WiFi.scanComplete() == -2) {
    WiFi.scanNetworks(true);
  }
  strcat(i2cstat, "sOk");
}
