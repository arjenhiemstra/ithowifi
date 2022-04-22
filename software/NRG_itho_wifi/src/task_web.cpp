#include "task_web.h"

#include "webroot/index_html_gz.h"
#include "webroot/controls_js_gz.h"
#include "webroot/pure_min_css_gz.h"
#include "webroot/zepto_min_js_gz.h"
#include "webroot/favicon_png_gz.h"

#define TASK_WEB_PRIO 5

    // globals
    TaskHandle_t xTaskWebHandle;
uint32_t TaskWebHWmark = 0;
bool sysStatReq = false;
int MQTT_conn_state = -5;
int MQTT_conn_state_new = 0;
unsigned long lastMQTTReconnectAttempt = 0;
bool dontReconnectMQTT = false;
bool updateMQTTihtoStatus = false;

AsyncWebServer server(80);

// locals
Ticker DelayedReq;

StaticTask_t xTaskWebBuffer;
StackType_t xTaskWebStack[STACK_SIZE_LARGE];

unsigned long lastSysMessage = 0;
unsigned long previousUpdate = 0;
String debugVal = "";
bool onOTA = false;
bool TaskWebStarted = false;

void startTaskWeb()
{
  xTaskWebHandle = xTaskCreateStaticPinnedToCore(
      TaskWeb,
      "TaskWeb",
      STACK_SIZE_LARGE,
      (void *)1,
      TASK_WEB_PRIO,
      xTaskWebStack,
      &xTaskWebBuffer,
      CONFIG_ARDUINO_RUNNING_CORE);
}

void TaskWeb(void *pvParameters)
{
  configASSERT((uint32_t)pvParameters == 1UL);
  Ticker TaskTimeout;

  ArduinoOTAinit();

  websocketInit();

  webServerInit();

  MDNSinit();

  TaskInitReady = true;

  esp_task_wdt_add(NULL);

  for (;;)
  {
    yield();
    esp_task_wdt_reset();

    TaskTimeout.once_ms(3000, []()
                        { logInput("Warning: Task Web timed out!"); });

    execWebTasks();

    TaskWebHWmark = uxTaskGetStackHighWaterMark(NULL);
    vTaskDelay(25 / portTICK_PERIOD_MS);
  }
  mg_mgr_free(&mgr);
  // else delete task
  vTaskDelete(NULL);
}

void execWebTasks()
{
  ArduinoOTA.handle();
  // ws.cleanupClients();
  mg_mgr_poll(&mgr, 1000);
  if (millis() - previousUpdate >= 5000 || sysStatReq)
  {
    if (millis() - lastSysMessage >= 1000 && !onOTA)
    { // rate limit messages to once a second
      sysStatReq = false;
      lastSysMessage = millis();
      // remotes.llModeTimerUpdated = false;
      previousUpdate = millis();
      sys.updateFreeMem();
      jsonSystemstat();
    }
  }
}

void ArduinoOTAinit()
{

  //  events.onConnect([](AsyncEventSourceClient * client) {
  //    client->send("hello!", NULL, millis(), 1000);
  //  });
  //  server.addHandler(&events);

  // Send OTA events to the browser
  ArduinoOTA.setHostname(hostName());
  ArduinoOTA
      .onStart([]()
               {

    static char buf[128] = "";
    strcat(buf, "Firmware update: ");
    
    if (ArduinoOTA.getCommand() == U_FLASH)
      strncat(buf, "sketch", sizeof(buf) - strlen(buf) - 1);
    else // U_SPIFFS
      strncat(buf, "filesystem", sizeof(buf) - strlen(buf) - 1);
    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    logInput(buf);
    delay(250);
    
    ACTIVE_FS.end();
    onOTA = true;
    dontSaveConfig = true;
    dontReconnectMQTT = true;
    mqttClient.disconnect();

    detachInterrupt(ITHO_IRQ_PIN);

    TaskCC1101Timeout.detach();
    TaskConfigAndLogTimeout.detach();
    TaskMQTTTimeout.detach();

    vTaskSuspend( xTaskCC1101Handle );
    vTaskSuspend( xTaskMQTTHandle );
    vTaskSuspend( xTaskConfigAndLogHandle );

    esp_task_wdt_delete( xTaskCC1101Handle );
    esp_task_wdt_delete( xTaskMQTTHandle );
    esp_task_wdt_delete( xTaskConfigAndLogHandle ); })
      .onEnd([]()
             { Serial.println("\nEnd"); })
      .onProgress([](unsigned int progress, unsigned int total)
                  { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); })
      .onError([](ota_error_t error)
               {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed"); });
  ArduinoOTA.begin();
  //
  //  ArduinoOTA.onStart([]() {
  //    Serial.end();
  //    events.send("Update Start", "ota");
  //  });
  //  ArduinoOTA.onEnd([]() {
  //    events.send("Update End", "ota");
  //  });
  //  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
  //    char p[32];
  //    sprintf(p, "Progress: %u%%\n", (progress / (total / 100)));
  //  });
  //  ArduinoOTA.onError([](ota_error_t error) {
  //    if (error == OTA_AUTH_ERROR)
  //      events.send("Auth Failed", "ota");
  //    else if (error == OTA_BEGIN_ERROR)
  //      events.send("Begin Failed", "ota");
  //    else if (error == OTA_CONNECT_ERROR)
  //      events.send("Connect Failed", "ota");
  //    else if (error == OTA_RECEIVE_ERROR)
  //      events.send("Recieve Failed", "ota");
  //    else if (error == OTA_END_ERROR)
  //      events.send("End Failed", "ota");
  //  });
  //  ArduinoOTA.setHostname(hostName());
  //  ArduinoOTA.begin();
}

void webServerInit()
{

  // upload a file to /upload
  server.on(
      "/upload", HTTP_POST, [](AsyncWebServerRequest *request)
      {
    if (systemConfig.syssec_web) {
      if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
        return request->requestAuthentication();
    }
    request->send(200); },
      [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
      {
        // Handle upload
      });

  if (systemConfig.syssec_edit)
  {
    server.addHandler(new SPIFFSEditor(ACTIVE_FS, systemConfig.sys_username, systemConfig.sys_password));
  }
  else
  {
    server.addHandler(new SPIFFSEditor(ACTIVE_FS, "", ""));
  }

  // css_code
  server.on("/pure-min.css", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css", pure_min_css_gz, pure_min_css_gz_len);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response); });

  // javascript files
  server.on("/js/zepto.min.js", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "application/javascript", zepto_min_js_gz, zepto_min_js_gz_len);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response); });

  server.on("/js/controls.js", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "application/javascript", controls_js_gz, controls_js_gz_len);
    response->addHeader("Server", "Itho WiFi Web Server");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response); });

  server.on("/js/general.js", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    AsyncResponseStream *response = request->beginResponseStream("application/javascript");
    response->addHeader("Server", "Itho WiFi Web Server");

    response->print("var on_ap = false; var hostname = '");
    response->print(hostName());
    response->print("'; var fw_version = '");
    response->print(FWVERSION);
    response->print("'; var hw_revision = '");
    response->print(HWREVISION);
    response->print("'; $(document).ready(function() { $('#headingindex').text(hostname); $('#headingindex').attr('href', 'http://' + hostname + '.local'); $('#main').append(html_index); });");

    request->send(response); })
      .setFilter(ON_STA_FILTER);

  server.on("/js/general.js", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    AsyncResponseStream *response = request->beginResponseStream("application/javascript");
    response->addHeader("Server", "Itho WiFi Web Server");

    response->print("var on_ap = true; var hostname = '");
    response->print(hostName());
    response->print("'; var fw_version = '");
    response->print(FWVERSION);
    response->print("'; var hw_revision = '");
    response->print(HWREVISION);
    response->print("'; $(document).ready(function() { $('#headingindex').text(hostname); $('#headingindex').attr('href', 'http://' + hostname + '.local'); $('#main').append(html_wifisetup); });");

    request->send(response); })
      .setFilter(ON_AP_FILTER);

  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "image/png", favicon_png_gz, favicon_png_gz_len);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response); });

  // HTML pages
  server.rewrite("/", "/index.htm");
  server.on("/index.htm", HTTP_ANY, [](AsyncWebServerRequest *request)
            {
    if (systemConfig.syssec_web) {
      if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
        return request->requestAuthentication();
    }
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html_gz, index_html_gz_len);
    response->addHeader("Server", "Itho WiFi Web Server");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response); });
  server.on("/api.html", HTTP_GET, handleAPI);
  server.on("/debug", HTTP_GET, handleDebug);

  // Log file download
  server.on("/curlog", HTTP_GET, handleCurLogDownload);
  server.on("/prevlog", HTTP_GET, handlePrevLogDownload);

  // HTTP basic authentication
  server.on("/login", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    if (systemConfig.syssec_web) {
      if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
        return request->requestAuthentication();
    }
    request->send(200, "text/plain", "Login Success!"); });
  // Simple Firmware Update Form
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    if (systemConfig.syssec_web) {
      if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
        return request->requestAuthentication();
    }
    request->send(200, "text/html", "<form method='POST' action='/update' "
                  "enctype='multipart/form-data'><input "
                  "type='file' name='update'><input "
                  "type='submit' value='Update'></form>"); });

  server.on(
      "/update", HTTP_POST, [](AsyncWebServerRequest *request)
      {
    if (systemConfig.syssec_web) {
      if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
        return request->requestAuthentication();
    }
    shouldReboot = !Update.hasError();
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", shouldReboot ? "OK" : "FAIL");
    response->addHeader("Connection", "close");
    request->send(response); },
      [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
      {
        if (!index)
        {
#if defined(ENABLE_SERIAL)
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

          detachInterrupt(ITHO_IRQ_PIN);

          TaskCC1101Timeout.detach();
          TaskConfigAndLogTimeout.detach();
          TaskMQTTTimeout.detach();

          vTaskSuspend(xTaskCC1101Handle);
          vTaskSuspend(xTaskMQTTHandle);
          vTaskSuspend(xTaskConfigAndLogHandle);

          esp_task_wdt_delete(xTaskCC1101Handle);
          esp_task_wdt_delete(xTaskMQTTHandle);
          esp_task_wdt_delete(xTaskConfigAndLogHandle);

#if defined(ENABLE_SERIAL)
          D_LOG("Tasks detached\n");
#endif

          if (!Update.begin(UPDATE_SIZE_UNKNOWN))
          {
#if defined(ENABLE_SERIAL)
            Update.printError(Serial);
#endif
          }
        }
        if (!Update.hasError())
        {
          if (Update.write(data, len) != len)
          {
#if defined(ENABLE_SERIAL)
            Update.printError(Serial);
#endif
          }
        }
        if (final)
        {
          if (Update.end(true))
          {
#if defined(ENABLE_SERIAL)
            D_LOG("Update Success: %uB\n", index + len);
#endif
          }
          else
          {
#if defined(ENABLE_SERIAL)
            Update.printError(Serial);
#endif
          }
          onOTA = false;
        }
      });
  Update.onProgress(otaWSupdate);

  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    if (systemConfig.syssec_web) {
      if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
        return request->requestAuthentication();
    }
    logMessagejson("Reset requested. Device will reboot in a few seconds...", WEBINTERFACE);
    delay(200);
    shouldReboot = true; });

  // Catch-All Handlers
  // Any request that can not find a Handler that canHandle it
  // ends in the callbacks below.
  server.onNotFound([](AsyncWebServerRequest *request)
                    {
    //Handle Unknown Request
    request->send(404); });

  server.begin();

  logInput("Webserver: started");
}

void MDNSinit()
{

  MDNS.begin(hostName());
  mdns_instance_name_set(hostName());

  MDNS.addService("http", "tcp", 80);

  char logBuff[LOG_BUF_SIZE] = "";
  logInput("mDNS: started");

  sprintf(logBuff, "Hostname: %s", hostName());
  logInput(logBuff);
}
void handleAPI(AsyncWebServerRequest *request)
{
  bool parseOK = false;

  int params = request->params();

  if (systemConfig.syssec_api)
  {
    bool username = false;
    bool password = false;

    for (int i = 0; i < params; i++)
    {
      AsyncWebParameter *p = request->getParam(i);
      if (strcmp(p->name().c_str(), "username") == 0)
      {
        if (strcmp(p->value().c_str(), systemConfig.sys_username) == 0)
        {
          username = true;
        }
      }
      else if (strcmp(p->name().c_str(), "password") == 0)
      {
        if (strcmp(p->value().c_str(), systemConfig.sys_password) == 0)
        {
          password = true;
        }
      }
    }
    if (!username || !password)
    {
      request->send(200, "text/html", "AUTHENTICATION FAILED");
      return;
    }
  }

  const char *speed = nullptr;
  const char *timer = nullptr;
  const char *vremotecmd = nullptr;
  const char *vremoteidx = nullptr;
  const char *vremotename = nullptr;

  for (int i = 0; i < params; i++)
  {
    AsyncWebParameter *p = request->getParam(i);

    if (strcmp(p->name().c_str(), "get") == 0)
    {
      AsyncWebParameter *p = request->getParam("get");
      if (strcmp(p->value().c_str(), "currentspeed") == 0)
      {
        char ithoval[5];
        sprintf(ithoval, "%d", ithoCurrentVal);
        request->send(200, "text/html", ithoval);
        return;
      }
      else if (strcmp(p->value().c_str(), "queue") == 0)
      {
        AsyncResponseStream *response = request->beginResponseStream("text/html");
        StaticJsonDocument<1000> root;
        JsonObject obj = root.to<JsonObject>(); // Fill the object
        ithoQueue.get(obj);
        obj["ithoSpeed"] = ithoQueue.ithoSpeed;
        obj["ithoOldSpeed"] = ithoQueue.ithoOldSpeed;
        obj["fallBackSpeed"] = ithoQueue.fallBackSpeed;
        serializeJson(root, *response);
        request->send(response);

        return;
      }
      else if (strcmp(p->value().c_str(), "ithostatus") == 0)
      {
        AsyncResponseStream *response = request->beginResponseStream("text/html");
        DynamicJsonDocument doc(6000);

        if (doc.capacity() == 0)
        {
          logInput("MQTT: JsonDocument memory allocation failed (html api)");
          return;
        }

        JsonObject root = doc.to<JsonObject>();
        getIthoStatusJSON(root);

        serializeJson(root, *response);
        request->send(response);

        return;
      }
      else if (strcmp(p->value().c_str(), "lastcmd") == 0)
      {
        AsyncResponseStream *response = request->beginResponseStream("text/html");
        DynamicJsonDocument doc(1000);

        JsonObject root = doc.to<JsonObject>();
        getLastCMDinfoJSON(root);

        serializeJson(root, *response);
        request->send(response);

        return;
      }
      else if (strcmp(p->value().c_str(), "remotesinfo") == 0)
      {
        AsyncResponseStream *response = request->beginResponseStream("text/html");
        DynamicJsonDocument doc(2000);

        JsonObject root = doc.to<JsonObject>();

        getRemotesInfoJSON(root);

        serializeJson(root, *response);
        request->send(response);

        return;
      }
    }
    else if (strcmp(p->name().c_str(), "command") == 0)
    {
      parseOK = ithoExecCommand(p->value().c_str(), HTMLAPI);
    }
    else if (strcmp(p->name().c_str(), "vremote") == 0)
    {
      vremotecmd = p->value().c_str();
    }
    else if (strcmp(p->name().c_str(), "vremotecmd") == 0)
    {
      vremotecmd = p->value().c_str();
    }
    else if (strcmp(p->name().c_str(), "vremoteindex") == 0)
    {
      vremoteidx = p->value().c_str();
    }
    else if (strcmp(p->name().c_str(), "vremotename") == 0)
    {
      vremotename = p->value().c_str();
    }
    else if (strcmp(p->name().c_str(), "speed") == 0)
    {
      speed = p->value().c_str();
    }
    else if (strcmp(p->name().c_str(), "timer") == 0)
    {
      timer = p->value().c_str();
    }
    else if (strcmp(p->name().c_str(), "debug") == 0)
    {
      if (strcmp(p->value().c_str(), "level0") == 0)
      {
        setRFdebugLevel(0);
        parseOK = true;
      }
      if (strcmp(p->value().c_str(), "level1") == 0)
      {
        setRFdebugLevel(1);
        parseOK = true;
      }
      if (strcmp(p->value().c_str(), "level2") == 0)
      {
        setRFdebugLevel(2);
        parseOK = true;
      }
      if (strcmp(p->value().c_str(), "level3") == 0)
      {
        setRFdebugLevel(3);
        parseOK = true;
      }
      if (strcmp(p->value().c_str(), "reboot") == 0)
      {
        shouldReboot = true;
        logMessagejson("Reboot requested", WEBINTERFACE);
        parseOK = true;
      }
    }
  }

  if (vremotecmd != nullptr || vremoteidx != nullptr || vremotename != nullptr)
  {
    if (vremotecmd != nullptr && (vremoteidx == nullptr && vremotename == nullptr))
    {
      parseOK = ithoI2CCommand(0, vremotecmd, HTMLAPI);
    }
    else
    {
      // exec command for specific vremote
      int index = -1;
      if (vremoteidx != nullptr)
      {
        if (strcmp(vremoteidx, "0") == 0)
        {
          index = 0;
        }
        else
        {
          index = atoi(vremoteidx);
          if (index == 0)
            index = -1;
        }
      }
      if (vremotename != nullptr)
      {
        index = virtualRemotes.getRemoteIndexbyName(vremotename);
      }
      if (index == -1)
        parseOK = false;
      else
      {
        parseOK = ithoI2CCommand(index, vremotecmd, HTMLAPI);
      }
    }
  }
  else if (speed != nullptr || timer != nullptr)
  {
    // check speed & timer
    if (speed != nullptr && timer != nullptr)
    {
      parseOK = ithoSetSpeedTimer(speed, timer, HTMLAPI);
    }
    else if (speed != nullptr && timer == nullptr)
    {
      parseOK = ithoSetSpeed(speed, HTMLAPI);
    }
    else if (timer != nullptr && speed == nullptr)
    {
      parseOK = ithoSetTimer(timer, HTMLAPI);
    }
  }

  if (parseOK)
  {
    request->send(200, "text/html", "OK");
  }
  else
  {
    request->send(200, "text/html", "NOK");
  }
}
void handleDebug(AsyncWebServerRequest *request)
{
  if (systemConfig.syssec_web)
  {
    if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
      return request->requestAuthentication();
  }
  AsyncResponseStream *response = request->beginResponseStream("text/html");

  response->print("<div class=\"header\"><h1>Debug page</h1></div><br><br>");
  response->print("<div>Config version: ");
  response->print(CONFIG_VERSION);
  response->print("<br><br><span>Itho I2C connection status: </span><span id=\'ithoinit\'>unknown</span></div>");

  response->print("<br><span>File system: </span><span>");

  response->print(ACTIVE_FS.usedBytes());
  response->print(" bytes used / ");
  response->print(ACTIVE_FS.totalBytes());
  response->print(" bytes total</span><br><a href='#' class='pure-button' onclick=\"$('#main').empty();$('#main').append( html_edit );\">Edit filesystem</a>");
  response->print("<br><br><span>CC1101 task memory: </span><span>");
  response->print(TaskCC1101HWmark);
  response->print(" bytes free</span>");
  response->print("<br><span>MQTT task memory: </span><span>");
  response->print(TaskMQTTHWmark);
  response->print(" bytes free</span>");
  response->print("<br><span>Web task memory: </span><span>");
  response->print(TaskWebHWmark);
  response->print(" bytes free</span>");
  response->print("<br><span>Config and Log task memory: </span><span>");
  response->print(TaskConfigAndLogHWmark);
  response->print(" bytes free</span>");
  response->print("<br><span>SysControl task memory: </span><span>");
  response->print(TaskSysControlHWmark);
  response->print(" bytes free</span></div>");
  response->print("<br><br><div id='syslog_outer'><div style='display:inline-block;vertical-align:top;overflow:hidden;padding-bottom:5px;'>System Log:</div>");

  response->print("<div style='padding:10px;background-color:black;min-height:30vh;max-height:60vh;font: 0.9rem Inconsolata, monospace;border-radius:7px;overflow:auto'>");
  char link[24] = "";
  char linkcur[24] = "";

  if (ACTIVE_FS.exists("/logfile0.current.log"))
  {
    strlcpy(linkcur, "/logfile0.current.log", sizeof(linkcur));
    strlcpy(link, "/logfile1.log", sizeof(link));
  }
  else
  {
    strlcpy(linkcur, "/logfile1.current.log", sizeof(linkcur));
    strlcpy(link, "/logfile0.log", sizeof(link));
  }

  File file = ACTIVE_FS.open(linkcur, FILE_READ);
  while (file.available())
  {
    if (char(file.peek()) == '\n')
      response->print("<br>");
    response->print(char(file.read()));
  }
  file.close();

  response->print("</div><div style='padding-top:5px;'><a class='pure-button' href='/curlog'>Download current logfile</a>");

  if (ACTIVE_FS.exists(link))
  {
    response->print("&nbsp;<a class='pure-button' href='/prevlog'>Download previous logfile</a>");
  }

  response->print("</div></div><br><br><div id='rflog_outer' class='hidden'><div style='display:inline-block;vertical-align:top;overflow:hidden;padding-bottom:5px;'>RF Log:</div>");
  response->print("<div id='rflog' style='padding:10px;background-color:black;min-height:30vh;max-height:60vh;font: 0.9rem Inconsolata, monospace;border-radius:7px;overflow:auto'>");
  response->print("</div><div style='padding-top:5px;'><a href='#' class='pure-button' onclick=\"$('#rflog').empty()\">Clear</a></div></div></div>");
  response->print("<form class=\"pure-form pure-form-aligned\"><fieldset><legend><br>RF debug mode (only functional with active CC1101 RF module):</legend><br><button id=\"rfdebug-0\" class=\"pure-button pure-button-primary\">Off</button><br><br><button id=\"rfdebug-1\" class=\"pure-button pure-button-primary\">Level1</button>&nbsp;Level1 will show only known itho commands from all devices<br><br><button id=\"rfdebug-2\" class=\"pure-button pure-button-primary\">Level2</button>&nbsp;Level2 will show all received RF messages from devices joined to the add-on<br><br><button id=\"rfdebug-3\" class=\"pure-button pure-button-primary\">Level3</button>&nbsp;Level3 will show all received RF messages from all devices<br><br>");
  response->print("<form class=\"pure-form pure-form-aligned\"><fieldset><legend><br>Low level itho I2C commands:</legend><br><span>I2C virtual remote commands:</span><br><button id=\"ithobutton-low\" class=\"pure-button pure-button-primary\">Low</button>&nbsp;<button id=\"ithobutton-medium\" class=\"pure-button pure-button-primary\">Medium</button>&nbsp;<button id=\"ithobutton-high\" class=\"pure-button pure-button-primary\">High</button><br><br>");
  response->print("<button id=\"ithobutton-join\" class=\"pure-button pure-button-primary\">Join</button>&nbsp;<button id=\"ithobutton-leave\" class=\"pure-button pure-button-primary\">Leave</button><br><br>");
  response->print("<button id=\"ithobutton-type\" class=\"pure-button pure-button-primary\">Query Devicetype</button><br><span>Result:&nbsp;</span><span id=\'ithotype\'></span><br><br>");
  response->print("<button id=\"ithobutton-statusformat\" class=\"pure-button pure-button-primary\">Query Status Format</button><br><span>Result:&nbsp;</span><span id=\'ithostatusformat\'></span><br><br>");
  response->print("<button id=\"ithobutton-status\" class=\"pure-button pure-button-primary\">Query Status</button><br><span>Result:&nbsp;</span><span id=\'ithostatus\'></span><br><br>");
  response->print("<button id=\"button2410\" class=\"pure-button pure-button-primary\">Query 2410</button>setting index: <input id=\"itho_setting_id\" type=\"number\" min=\"0\" max=\"254\" size=\"6\" value=\"0\"><br><span>Result:&nbsp;</span><span id=\'itho2410\'></span><br><span>Current:&nbsp;</span><span id=\'itho2410cur\'></span><br><span>Minimum value:&nbsp;</span><span id=\'itho2410min\'></span><br><span>Maximum value:&nbsp;</span><span id=\'itho2410max\'></span><br><br>");
  response->print("<span style=\"color:red\">Warning!!<br> \"Set 2410\" changes the settings of your itho unit<br>Use with care and use only if you know what you are doing!</span><br>");
  response->print("<button id=\"button2410set\" class=\"pure-button pure-button-primary\">Set 2410</button>setting index: <input id=\"itho_setting_id_set\" type=\"number\" min=\"0\" max=\"254\" size=\"6\" value=\"0\"> setting value: <input id=\"itho_setting_value_set\" type=\"number\" min=\"-2147483647\" max=\"2147483647\" size=\"10\" value=\"0\"><br><span>Sent command:&nbsp;</span><span id=\'itho2410set\'></span><br><span>Result:&nbsp;</span><span id=\'itho2410setres\'></span><br>");
  response->print("<span style=\"color:red\">Warning!!</span><br><br>");
  response->print("<button id=\"ithobutton-31DA\" class=\"pure-button pure-button-primary\">Query 31DA</button><br><span>Result:&nbsp;</span><span id=\'itho31DA\'></span><br><br>");
  response->print("<button id=\"ithobutton-31D9\" class=\"pure-button pure-button-primary\">Query 31D9</button><br><span>Result:&nbsp;</span><span id=\'itho31D9\'></span><br><br>");
  response->print("<button id=\"ithobutton-10D0\" class=\"pure-button pure-button-primary\">Filter reset</button></fieldset></form><br>");

  response->print("<br><br>");

  request->send(response);

  DelayedReq.once(1, []()
                  { sysStatReq = true; });
}

void handleCurLogDownload(AsyncWebServerRequest *request)
{
  if (systemConfig.syssec_web)
  {
    if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
      return request->requestAuthentication();
  }
  char link[24] = "";
  if (ACTIVE_FS.exists("/logfile0.current.log"))
  {
    strlcpy(link, "/logfile0.current.log", sizeof(link));
  }
  else
  {
    strlcpy(link, "/logfile1.current.log", sizeof(link));
  }
  request->send(ACTIVE_FS, link, "", true);
}

void handlePrevLogDownload(AsyncWebServerRequest *request)
{
  if (systemConfig.syssec_web)
  {
    if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
      return request->requestAuthentication();
  }
  char link[24] = "";
  if (ACTIVE_FS.exists("/logfile0.current.log"))
  {
    strlcpy(link, "/logfile1.log", sizeof(link));
  }
  else
  {
    strlcpy(link, "/logfile0.log", sizeof(link));
  }
  request->send(ACTIVE_FS, link, "", true);
}

void jsonSystemstat()
{
  StaticJsonDocument<512> root;
  // JsonObject root = jsonBuffer.createObject();
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
