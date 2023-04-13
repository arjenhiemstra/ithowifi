#include "tasks/task_web.h"

#if MG_ENABLE_PACKED_FS == 0

#include "webroot/index_html_gz.h"
#include "webroot/edit_html_gz.h"
#include "webroot/controls_js_gz.h"
#include "webroot/pure_min_css_gz.h"
#include "webroot/zepto_min_js_gz.h"
#include "webroot/favicon_png_gz.h"

#endif

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
bool webauth_ok = false;

AsyncWebServer server(80);

// locals
StaticTask_t xTaskWebBuffer;
StackType_t xTaskWebStack[STACK_SIZE_MEDIUM];

unsigned long lastSysMessage = 0;
unsigned long previousUpdate = 0;
String debugVal = "";
bool onOTA = false;
bool TaskWebStarted = false;

static const struct packed_files_lst
{
  const char *name;
} packed_files_list[] = {
    "/",
    "/index.html",
    "/controls.js",
    "/edit.html",
    "/favicon.png",
    "/pure-min.css",
    "/zepto.min.js",
    ""};

// const char packed_files_list[] = {
//     "/index.html",
//     "/controls.js",
//     "/edit.html",
//     "/favicon.png",
//     "/pure-min.css",
//     "/zepto.min.js",
//     NULL,
// };

void startTaskWeb()
{
  xTaskWebHandle = xTaskCreateStaticPinnedToCore(
      TaskWeb,
      "TaskWeb",
      STACK_SIZE_MEDIUM,
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

    TaskTimeout.once_ms(15000, []()
                        { W_LOG("Warning: Task Web timed out!"); });

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

  mg_mgr_poll(&mgr, 500);
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
                 static char buf[128]{};
                 strcat(buf, "Firmware update: ");

                 if (ArduinoOTA.getCommand() == U_FLASH)
                   strncat(buf, "sketch", sizeof(buf) - strlen(buf) - 1);
                 else // U_SPIFFS
                   strncat(buf, "filesystem", sizeof(buf) - strlen(buf) - 1);
                 // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
                 N_LOG(buf);
                 delay(250);

                 ACTIVE_FS.end();
                 onOTA = true;
                 dontSaveConfig = true;
                 dontReconnectMQTT = true;
                 mqttClient.disconnect();

                 if (xTaskCC1101Handle != NULL)
                 {
                   detachInterrupt(itho_irq_pin);
                   TaskCC1101Timeout.detach();
                   vTaskSuspend(xTaskCC1101Handle);
                   esp_task_wdt_delete(xTaskCC1101Handle);
                 }
                 if (xTaskMQTTHandle != NULL)
                 {
                   TaskMQTTTimeout.detach();
                   vTaskSuspend(xTaskMQTTHandle);
                   esp_task_wdt_delete(xTaskMQTTHandle);
                 }
                 if (xTaskConfigAndLogHandle != NULL)
                 {
                   TaskConfigAndLogTimeout.detach();
                   vTaskSuspend(xTaskConfigAndLogHandle);
                   esp_task_wdt_delete(xTaskConfigAndLogHandle);
                 } })
      .onEnd([]()
             { D_LOG("\nEnd"); })
      .onProgress([](unsigned int progress, unsigned int total)
                  { D_LOG("Progress: %u%%\r", (progress / (total / 100))); })
      .onError([](ota_error_t error)
               {
    D_LOG("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) D_LOG("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) D_LOG("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) D_LOG("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) D_LOG("Receive Failed");
    else if (error == OTA_END_ERROR)
      D_LOG("End Failed"); });
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
  //    snprintf(p, sizeof(p), "Progress: %u%%\n", (progress / (total / 100)));
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

#if defined MG_ENABLE_PACKED_FS && MG_ENABLE_PACKED_FS == 1
  mg_http_listen(&mgr, s_listen_on_http, httpEvent, &mgr); // Create HTTP listener
#else

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

  server.on("/edit", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    if (systemConfig.syssec_edit) {
      if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
      return request->requestAuthentication();
    }
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", edit_html_gz, edit_html_gz_len);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response); });

  server.on("/edit", HTTP_PUT, handleFileCreate);
  server.on("/edit", HTTP_DELETE, handleFileDelete);

  // run handleUpload function when any file is uploaded
  server.on(
      "/edit", HTTP_POST, [](AsyncWebServerRequest *request)
      { request->send(200); },
      handleUpload);

  server.on("/list", HTTP_GET, handleFileList);
  server.on("/status", HTTP_GET, handleStatus);

  // server.on("/crash", HTTP_GET, handleCoreCrash);
  server.on("/getcoredump", HTTP_GET, handleCoredumpDownload);

  // css_code
  server.on("/pure-min.css", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/css", pure_min_css_gz, pure_min_css_gz_len);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response); });

  // javascript files
  server.on("/zepto.min.js", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "application/javascript", zepto_min_js_gz, zepto_min_js_gz_len);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response); });

  server.on("/controls.js", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "application/javascript", controls_js_gz, controls_js_gz_len);
    response->addHeader("Server", "Itho WiFi Web Server");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response); });

  server.on("/general.js", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    AsyncResponseStream *response = request->beginResponseStream("application/javascript");
    response->addHeader("Server", "Itho WiFi Web Server");

    response->print("var on_ap = false; var hostname = '");
    response->print(hostName());
    response->print("'; var fw_version = '");
    response->print(FWVERSION);
    response->print("'; var hw_revision = '");
    response->print(hw_revision);
    response->print("'; var itho_pwm2i2c = '");
    response->print(systemConfig.itho_pwm2i2c);
    response->print("'; $(document).ready(function() { $('#headingindex').text(hostname); $('#headingindex').attr('href', 'http://' + hostname + '.local'); $('#main').append(html_index); });");

    request->send(response); })
      .setFilter(ON_STA_FILTER);

  server.on("/general.js", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    AsyncResponseStream *response = request->beginResponseStream("application/javascript");
    response->addHeader("Server", "Itho WiFi Web Server");

    response->print("var on_ap = true; var hostname = '");
    response->print(hostName());
    response->print("'; var fw_version = '");
    response->print(FWVERSION);
    response->print("'; var hw_revision = '");
    response->print(hw_revision);
    response->print("'; var itho_pwm2i2c = '");
    response->print(systemConfig.itho_pwm2i2c);
    response->print("'; $(document).ready(function() { $('#headingindex').text(hostname); $('#headingindex').attr('href', 'http://' + hostname + '.local'); $('#main').append(html_wifisetup); });");

    request->send(response); })
      .setFilter(ON_AP_FILTER);

  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    AsyncWebServerResponse *response = request->beginResponse_P(200, "image/png", favicon_png_gz, favicon_png_gz_len);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response); });

  // HTML pages
  server.rewrite("/", "/index.html");
  server.on("/index.html", HTTP_ANY, [](AsyncWebServerRequest *request)
            {
    if (systemConfig.syssec_web) {
      if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password)) {
        webauth_ok = false;
        return request->requestAuthentication();
      }
      else {
        webauth_ok = true;
      }
    }
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", index_html_gz, index_html_gz_len);
    response->addHeader("Server", "Itho WiFi Web Server");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response); });
  server.on("/api.html", HTTP_GET, handleAPI);

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
          D_LOG("Begin OTA update");
          onOTA = true;
          dontSaveConfig = true;
          dontReconnectMQTT = true;
          mqttClient.disconnect();
          i2c_sniffer_unload();

          content_len = request->contentLength();

          N_LOG("Firmware update: %s", filename.c_str());
          delay(1000);

          if (xTaskCC1101Handle != NULL)
          {
            detachInterrupt(itho_irq_pin);
            TaskCC1101Timeout.detach();
            vTaskSuspend(xTaskCC1101Handle);
            esp_task_wdt_delete(xTaskCC1101Handle);
          }
          if (xTaskMQTTHandle != NULL)
          {
            TaskMQTTTimeout.detach();
            vTaskSuspend(xTaskMQTTHandle);
            esp_task_wdt_delete(xTaskMQTTHandle);
          }
          if (xTaskConfigAndLogHandle != NULL)
          {
            TaskConfigAndLogTimeout.detach();
            vTaskSuspend(xTaskConfigAndLogHandle);
            esp_task_wdt_delete(xTaskConfigAndLogHandle);
          }

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
            D_LOG("Update Success: %uB", index + len);
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
    //--> download or not
    //--> Handle request
    //if not known file
    //Handle Unknown Request
    if (!handleFileRead(request))
      request->send(404); });

  server.begin();
#endif
  N_LOG("Webserver: started");
}

void MDNSinit()
{

  MDNS.begin(hostName());
  mdns_instance_name_set(hostName());

  MDNS.addService("http", "tcp", 80);

  N_LOG("mDNS: started");

  N_LOG("Hostname: %s", hostName());
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
  const char *rfremotecommand = nullptr;
  const char *rfremoteidx = nullptr;

  for (int i = 0; i < params; i++)
  {
    AsyncWebParameter *p = request->getParam(i);

    if (strcmp(p->name().c_str(), "get") == 0)
    {
      AsyncWebParameter *q = request->getParam("get");
      if (strcmp(q->value().c_str(), "currentspeed") == 0)
      {
        char ithoval[10]{};
        snprintf(ithoval, sizeof(ithoval), "%d", ithoCurrentVal);
        request->send(200, "text/html", ithoval);
        return;
      }
      else if (strcmp(q->value().c_str(), "queue") == 0)
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
      else if (strcmp(q->value().c_str(), "ithostatus") == 0)
      {
        AsyncResponseStream *response = request->beginResponseStream("text/html");
        DynamicJsonDocument doc(6000);

        if (doc.capacity() == 0)
        {
          E_LOG("MQTT: JsonDocument memory allocation failed (html api)");
          return;
        }

        JsonObject root = doc.to<JsonObject>();
        getIthoStatusJSON(root);

        serializeJson(root, *response);
        request->send(response);

        return;
      }
      else if (strcmp(q->value().c_str(), "lastcmd") == 0)
      {
        AsyncResponseStream *response = request->beginResponseStream("text/html");
        DynamicJsonDocument doc(1000);

        JsonObject root = doc.to<JsonObject>();
        getLastCMDinfoJSON(root);

        serializeJson(root, *response);
        request->send(response);

        return;
      }
      else if (strcmp(q->value().c_str(), "remotesinfo") == 0)
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
    else if (strcmp(p->name().c_str(), "getsetting") == 0)
    {
      if (systemConfig.api_settings == 0)
      {
        request->send(403, "text/html", "The settings API is disabled");
        return;
      }

      uint8_t idx = static_cast<uint8_t>(p->value().toInt());

      if (ithoSettingsArray == nullptr)
      {
        request->send(400, "text/html", "The Itho settings array is NULL");
        return;
      }

      if (idx >= currentIthoSettingsLength())
      {
        request->send(400, "text/html", "The setting index is invalid");
        return;
      }

      // Get the settings directly instead of scheduling them, that way we can
      // present the up-to-date settings in the response.
      resultPtr2410 = sendQuery2410(idx, true);
      ithoSettings *setting = &ithoSettingsArray[idx];
      double cur = 0.0;
      double min = 0.0;
      double max = 0.0;

      if (resultPtr2410 == nullptr)
      {
        request->send(500, "text/html", "The I2C command failed");
        return;
      }

      if (!decodeQuery2410(resultPtr2410, setting, &cur, &min, &max))
      {
        request->send(500, "text/html", "Failed to decode the setting's value");
        return;
      }

      StaticJsonDocument<128> doc;
      JsonObject root = doc.to<JsonObject>();

      root["current"] = cur;
      root["minimum"] = min;
      root["maximum"] = max;

      AsyncResponseStream *response = request->beginResponseStream("text/html");

      serializeJson(root, *response);
      request->send(response);
      return;
    }
    else if (strcmp(p->name().c_str(), "setsetting") == 0)
    {
      if (systemConfig.api_settings == 0)
      {
        request->send(403, "text/html", "The settings API is disabled");
        return;
      }

      AsyncWebParameter *val_param = request->getParam("value");

      if (val_param == nullptr)
      {
        request->send(400, "text/html", "The 'value' parameter is required");
        return;
      }

      if (ithoSettingsArray == nullptr)
      {
        request->send(400, "text/html", "The Itho settings array is NULL");
        return;
      }

      uint8_t idx = static_cast<uint8_t>(p->value().toInt());

      if (idx >= currentIthoSettingsLength())
      {
        request->send(400, "text/html", "The setting index is invalid");
        return;
      }

      ithoSettings *setting = &ithoSettingsArray[idx];
      resultPtr2410 = sendQuery2410(idx, true);
      double cur = 0.0;
      double min = 0.0;
      double max = 0.0;

      if (resultPtr2410 == nullptr)
      {
        request->send(500, "text/html", "The I2C command failed");
        return;
      }

      if (!decodeQuery2410(resultPtr2410, setting, &cur, &min, &max))
      {
        request->send(500, "text/html", "Failed to decode the setting's value");
        return;
      }

      const String &val = val_param->value();
      int32_t new_val = 0;
      double new_dval = val.toDouble();

      if (new_dval < min || new_dval > max)
      {
        request->send(400, "text/html", "The specified value falls outside of the allowed range");
        return;
      }

      if (setting->type == ithoSettings::is_float)
      {
        double dval = new_dval * setting->divider;

        switch (ithoSettingsArray[idx].length)
        {
          case 1:
            new_val = static_cast<int32_t>(static_cast<int8_t>(dval));
            break;
          case 2:
            new_val = static_cast<int32_t>(static_cast<int16_t>(dval));
            break;
          default:
            new_val = static_cast<int32_t>(dval);
            break;
        }
      }
      else
      {
        new_val = static_cast<int32_t>(new_dval);
      }

      // Settings are updated synchronously as API clients might expect the
      // response to not return until the settings are up to date (this is also
      // easier to work with).
      if (setSetting2410(idx, new_val, true))
      {
        request->send(200, "OK");
      }
      else
      {
        request->send(500, "text/html", "The I2C command failed");
      }

      return;
    }
    else if (strcmp(p->name().c_str(), "command") == 0)
    {
      parseOK = ithoExecCommand(p->value().c_str(), HTMLAPI);
    }
    else if (strcmp(p->name().c_str(), "rfcommand") == 0)
    {
      rfremotecommand = p->value().c_str();
      // parseOK = ithoExecRFCommand(p->value().c_str(), HTMLAPI);
    }
    else if (strcmp(p->name().c_str(), "rfremoteidx") == 0)
    {
      rfremoteidx = p->value().c_str();
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
    else if (strcmp(p->name().c_str(), "i2csniffer") == 0)
    {
      if (strcmp(p->value().c_str(), "on") == 0)
      {
        i2c_sniffer_enable();
        i2c_safe_guard.sniffer_enabled = true;
        i2c_safe_guard.sniffer_web_enabled = true;
        parseOK = true;
      }
      else if (strcmp(p->value().c_str(), "off") == 0)
      {
        i2c_sniffer_disable();
        i2c_safe_guard.sniffer_enabled = false;
        i2c_safe_guard.sniffer_web_enabled = false;
        parseOK = true;
      }
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
      ithoI2CCommand(0, vremotecmd, HTMLAPI);

      parseOK = true;
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
        ithoI2CCommand(index, vremotecmd, HTMLAPI);

        parseOK = true;
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
  else if (rfremotecommand != nullptr || rfremoteidx != nullptr)
  {
    uint8_t idx = 0;
    if (rfremoteidx != nullptr)
    {
      idx = strtoul(rfremoteidx, NULL, 10);
    }
    if (rfremotecommand != nullptr)
    {
      parseOK = ithoExecRFCommand(idx, rfremotecommand, HTMLAPI);
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

void handleCoreCrash(AsyncWebServerRequest *request)
{
  request->send(200);
  D_LOG("Triggering core crash...");
  assert(0);
}

void handleCoredumpDownload(AsyncWebServerRequest *request)
{
  // httpd_resp_set_type(req, "application/octet-stream");
  // httpd_resp_set_hdr(req, "Content-Disposition",
  //                    "attachment;filename=core.bin");

  esp_partition_iterator_t partition_iterator = esp_partition_find(
      ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_COREDUMP, "coredump");

  const esp_partition_t *partition = esp_partition_get(partition_iterator);

  esp_core_dump_summary_t *summary = static_cast<esp_core_dump_summary_t *>(malloc(sizeof(esp_core_dump_summary_t)));

  if (summary)
  {
    esp_err_t err = esp_core_dump_get_summary(summary);
    if (err == ESP_OK)
    {
      D_LOG("Getting core dump summary ok.");
    }
    else
    {
      D_LOG("Getting core dump summary not ok. Error: %d\n", (int)err);
      D_LOG("Probably no coredump present yet.\n");
      D_LOG("esp_core_dump_image_check() = %s\n", esp_core_dump_image_check() ? "OK" : "NOK");
    }
    free(summary);
  }

  int file_size = 65536;
  int maxLen = 1024;
  char buffer[maxLen];

  AsyncWebServerResponse *response = request->beginChunkedResponse("application/octet-stream", [partition, file_size, &buffer](uint8_t *buffer, size_t maxLen, size_t alreadySent) -> size_t
                                                                   {
      if (file_size - alreadySent >= maxLen) {
        ESP_ERROR_CHECK(
            esp_partition_read(partition, alreadySent, buffer, maxLen));
        return maxLen;
      } else {
        ESP_ERROR_CHECK(esp_partition_read(partition, alreadySent, buffer,
                                           file_size - alreadySent));
        //memcpy(buffer, fileArray + alreadySent, fileSize - alreadySent);
        return file_size - alreadySent;
      } });

  response->addHeader("Content-Disposition", "attachment; filename=\"coredump.bin\"");
  response->addHeader("Transfer-Encoding", "chunked");
  request->send(response);
}

void handleCurLogDownload(AsyncWebServerRequest *request)
{
  if (systemConfig.syssec_web)
  {
    if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
      return request->requestAuthentication();
  }
  char link[24]{};
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
  char link[24]{};
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

void handleFileCreate(AsyncWebServerRequest *request)
{
  if (systemConfig.syssec_edit)
  {
    if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
    {
      request->send(401);
      return;
    }
  }
  D_LOG("handleFileCreate called");

  String path;
  String src;
  int params = request->params();
  for (int i = 0; i < params; i++)
  {
    AsyncWebParameter *p = request->getParam(i);

    if (strcmp(p->name().c_str(), "path") == 0)
    {
      D_LOG("handleFileCreate path found ('%s')", p->value().c_str());
      path = p->value().c_str();
    }
    if (strcmp(p->name().c_str(), "src") == 0)
    {
      D_LOG("handleFileCreate src found ('%s')", p->value().c_str());
      src = p->value().c_str();
    }
    // D_LOG("Param[%d] (name:'%s', value:'%s'", i, p->name().c_str(), p->value().c_str());
  }

  if (path.isEmpty())
  {
    request->send(400, "text/plain", "PATH ARG MISSING");
    return;
  }
  if (path == "/")
  {
    request->send(400, "text/plain", "BAD PATH");
    return;
  }

  if (src.isEmpty())
  {
    // No source specified: creation
    D_LOG("handleFileCreate: %s", path.c_str());
    if (path.endsWith("/"))
    {
      // Create a folder
      path.remove(path.length() - 1);
      if (!ACTIVE_FS.mkdir(path))
      {
        request->send(500, "text/plain", "MKDIR FAILED");
        return;
      }
    }
    else
    {
      // Create a file
      File file = ACTIVE_FS.open(path, "w");
      if (file)
      {
        file.write(0);
        file.close();
      }
      else
      {
        request->send(500, "text/plain", "CREATE FAILED");
        return;
      }
    }
    request->send(200, "text/plain", path.c_str());
  }
  else
  {
    // Source specified: rename
    if (src == "/")
    {
      request->send(400, "text/plain", "BAD SRC");
      return;
    }
    if (!ACTIVE_FS.exists(src))
    {
      request->send(400, "text/plain", "BSRC FILE NOT FOUND");
      return;
    }

    D_LOG("handleFileCreate: %s from %s", path.c_str(), src.c_str());
    if (path.endsWith("/"))
    {
      path.remove(path.length() - 1);
    }
    if (src.endsWith("/"))
    {
      src.remove(src.length() - 1);
    }
    if (!ACTIVE_FS.rename(src, path))
    {
      request->send(500, "text/plain", "RENAME FAILED");
      return;
    }
    request->send(200);
  }
}

void handleFileDelete(AsyncWebServerRequest *request)
{
  if (systemConfig.syssec_edit)
  {
    if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
    {
      request->send(401);
      return;
    }
  }
  String path;

  D_LOG("handleFileDelete called");

  int params = request->params();
  for (int i = 0; i < params; i++)
  {
    AsyncWebParameter *p = request->getParam(i);

    if (strcmp(p->name().c_str(), "path") == 0)
    {
      path = p->value().c_str();
      D_LOG("handleFileDelete path found ('%s')", p->value().c_str());
    }
  }

  if (path.isEmpty() || path == "/")
  {
    request->send(500, "text/plain", "BAD PATH");
    return;
  }
  if (!ACTIVE_FS.exists(path))
  {
    request->send(500, "text/plain", "FILE NOT FOUND");
    return;
  }

  File root = ACTIVE_FS.open(path, "r");
  // If it's a plain file, delete it
  if (!root.isDirectory())
  {
    root.close();
    ACTIVE_FS.remove(path);
  }
  else
  {
    ACTIVE_FS.rmdir(path);
  }
  request->send(200);
}

void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
  if (systemConfig.syssec_edit)
  {
    if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
      return request->requestAuthentication();
  }
  String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url() + "\n";
  D_LOG(logmessage.c_str());

  if (!index)
  {
    logmessage = "Upload Start: " + String(filename) + "\n";
    // open the file on first call and store the file handle in the request object
    request->_tempFile = ACTIVE_FS.open("/" + filename, "w");
    D_LOG(logmessage.c_str());
  }

  if (len)
  {
    // stream the incoming chunk to the opened file
    request->_tempFile.write(data, len);
    logmessage = "Writing file: " + String(filename) + " index=" + String(index) + " len=" + String(len) + "\n";
    D_LOG(logmessage.c_str());
  }

  if (final)
  {
    logmessage = "Upload Complete: " + String(filename) + ",size: " + String(index + len) + "\n";
    // close the file handle as the upload is now done
    request->_tempFile.close();
    D_LOG(logmessage.c_str());
    request->redirect("/");
  }
}

/*
    Read the given file from the filesystem and stream it back to the client
*/
bool handleFileRead(AsyncWebServerRequest *request)
{
  if (systemConfig.syssec_edit)
  {
    if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
    {
      request->send(401);
      return false;
    }
  }
  String path = request->url();

  String pathWithGz = path + ".gz";
  if (ACTIVE_FS.exists(pathWithGz) || ACTIVE_FS.exists(path))
  {
    D_LOG("file found ('%s')", path.c_str());

    if (ACTIVE_FS.exists(pathWithGz))
    {
      path += ".gz";
    }
    request->send(ACTIVE_FS, path.c_str(), getContentType(request->hasParam("download"), path.c_str()), request->hasParam("download"));

    pathWithGz = String();
    path = String();
    return true;
  }
  pathWithGz = String();
  path = String();
  return false;
}

void handleStatus(AsyncWebServerRequest *request)
{
  if (systemConfig.syssec_edit)
  {
    if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
    {
      request->send(401);
      return;
    }
  }
  size_t totalBytes = ACTIVE_FS.totalBytes();
  size_t usedBytes = ACTIVE_FS.usedBytes();

  String json;
  json.reserve(128);
  json = "{\"type\":\"Filesystem\", \"isOk\":";

  json += "\"true\", \"totalBytes\":\"";
  json += totalBytes;
  json += "\", \"usedBytes\":\"";
  json += usedBytes;
  json += "\"";

  json += ",\"unsupportedFiles\":\"\"}";

  request->send(200, "application/json", json);
  json = String();
}

void handleFileList(AsyncWebServerRequest *request)
{
  if (systemConfig.syssec_edit)
  {
    if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
    {
      request->send(401);
      return;
    }
  }
  if (!request->hasParam("dir"))
  {
    request->send(500, "text/plain", "BAD ARGS\n");
    return;
  }

  String path = request->getParam("dir")->value();
  File root = ACTIVE_FS.open(path);
  path = String();

  String output = "[";
  if (root.isDirectory())
  {
    File file = root.openNextFile();
    while (file)
    {
      if (output != "[")
      {
        output += ',';
      }
      output += "{\"type\":\"";
      output += (file.isDirectory()) ? "dir" : "file";
      output += "\",\"name\":\"";
      output += String(file.path()).substring(1);
      output += "\",\"size\":\"";
      output += String(file.size());
      output += "\"}";
      file = root.openNextFile();
    }
  }
  output += "]";

  root.close();

  request->send(200, "application/json", output);
  output = String();
}

bool prevlog_available()
{
  if (ACTIVE_FS.exists("/logfile0.log") || ACTIVE_FS.exists("/logfile1.log"))
    return true;

  return false;
}

const char *getContentType(bool download, const char *filename)
{
  if (download)
    return "application/octet-stream";
  else if (strstr(filename, ".htm"))
    return "text/html";
  else if (strstr(filename, ".html"))
    return "text/html";
  else if (strstr(filename, ".log"))
    return "text/plain";
  else if (strstr(filename, ".css"))
    return "text/css";
  else if (strstr(filename, ".sass"))
    return "text/css";
  else if (strstr(filename, ".js"))
    return "application/javascript";
  else if (strstr(filename, ".png"))
    return "image/png";
  else if (strstr(filename, ".svg"))
    return "image/svg+xml";
  else if (strstr(filename, ".gif"))
    return "image/gif";
  else if (strstr(filename, ".jpg"))
    return "image/jpeg";
  else if (strstr(filename, ".ico"))
    return "image/x-icon";
  else if (strstr(filename, ".xml"))
    return "text/xml";
  else if (strstr(filename, ".pdf"))
    return "application/x-pdf";
  else if (strstr(filename, ".zip"))
    return "application/x-zip";
  else if (strstr(filename, ".gz"))
    return "application/x-gzip";
  return "text/plain";
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

// FIXME: below code crashes the ESP32 on bigger files due to a watchdog TG1WDT_SYS_RESET timeout, probably due to a too long time it takes to access the file
// SPIFFS is flat, so tell Mongoose that the FS root is a directory
// This cludge is not required for filesystems with directory support
// int my_stat(const char *path, size_t *size, time_t *mtime)
// {
//   int flags = mg_fs_posix.st(path, size, mtime);
//   if (strcmp(path, "/littlefs") == 0)
//     flags |= MG_FS_DIR;
//   return flags;
// }

// void httpEvent(struct mg_connection *c, int ev, void *ev_data, void *fn_data)
// {

//   if (ev == MG_EV_HTTP_MSG)
//   {
//     struct mg_http_message *hm = (mg_http_message *)ev_data;
//     if (mg_http_match_uri(hm, "/curlog"))
//     {

//       char url[24]{};
//       size_t len;

//       if (ACTIVE_FS.exists("/logfile0.current.log"))
//       {
//         len = strlcpy(url, "/logfile0.current.log", sizeof(url));
//       }
//       else
//       {
//         len = strlcpy(url, "/logfile1.current.log", sizeof(url));
//       }

//       struct mg_http_message mg;
//       mg.uri.ptr = &url[0];
//       mg.uri.len = len;

//       char buf[64]{};
//       int urilen = mg.uri.len + 1;
//       strlcpy(buf, mg.uri.ptr, urilen > sizeof(buf) ? sizeof(buf) : urilen);
//       D_LOG("File '%s' requested", buf);
//     }
//     else
//     {
//       // mg_http_reply(c, 404, "", "Not found: %d\n", MG_PATH_MAX);
//       // return;
//       struct mg_fs fs = mg_fs_posix;
//       fs.st = my_stat;
//       struct mg_http_serve_opts opts = {.root_dir = "/littlefs", .fs = &fs};
//       // opts.fs = NULL;

//       delay(1);
//       mg_http_serve_dir(c, hm, &opts);
//     }
//   }

//   (void)fn_data;
// }

#if defined MG_ENABLE_PACKED_FS && MG_ENABLE_PACKED_FS == 1
void httpEvent(struct mg_connection *c, int ev, void *ev_data, void *fn_data)
{
  if (ev == MG_EV_HTTP_MSG)
  {
    D_LOG("MG_EV_HTTP_MSG");
    struct mg_http_message *hm = (struct mg_http_message *)ev_data;
    if (mg_http_match_uri(hm, "/general.js"))
    {
      D_LOG("URI /general.js");
      mg_http_reply(c, 200, "Content-Type: application/javascript\r\n",
                    "var on_ap = %s; var hostname = '%s'; var fw_version = '%s'; var hw_revision = '%s'; var itho_pwm2i2c = '%d'; $(document).ready(function() { $('#headingindex').text(hostname); $('#headingindex').attr('href', 'http://' + hostname + '.local'); $('#main').append(html_index); });\n ",
                    wifiModeAP
                        ? "true"
                        : "false",
                    hostName(), FWVERSION, hw_revision, systemConfig.itho_pwm2i2c);
    }
    else if (mg_http_match_uri(hm, "/list"))
    {
      D_LOG("URI /list");
      mg_handleFileList(c, ev, ev_data, fn_data);
    }
    else if (mg_http_match_uri(hm, "/status"))
    {
      D_LOG("URI /status");
      mg_handleStatus(c, ev, ev_data, fn_data);
    }
    else if (mg_http_match_uri(hm, "/api.html"))
    {
      D_LOG("URI /api.html");
      // handleAPI
    }
    else if (mg_http_match_uri(hm, "/curlog"))
    {
      D_LOG("URI /curlog");
      char url[24]{};

      if (ACTIVE_FS.exists("/logfile0.current.log"))
      {
        strlcpy(url, "/logfile0.current.log", sizeof(url));
      }
      else
      {
        strlcpy(url, "/logfile1.current.log", sizeof(url));
      }

      struct mg_http_message mg;
      mg.uri.ptr = &url[0];
      mg.uri.len = strlen(url);

      char buf[64]{};
      int urilen = mg.uri.len + 1;
      strlcpy(buf, mg.uri.ptr, urilen > sizeof(buf) ? sizeof(buf) : urilen);
      D_LOG("File '%s' requested", buf);

      mg_serve_fs(c, (void *)&mg, true);
    }
    else if (mg_http_match_uri(hm, "/prevlog"))
    {
      D_LOG("URI /prevlog");

      char url[24]{};
      size_t len;

      if (ACTIVE_FS.exists("/logfile0.current.log"))
      {
        len = strlcpy(url, "/logfile1.log", sizeof(url));
      }
      else
      {
        len = strlcpy(url, "/logfile0.log", sizeof(url));
      }

      struct mg_http_message mg;
      mg.uri.ptr = &url[0];
      mg.uri.len = len;

      char buf[64]{};
      int urilen = mg.uri.len + 1;
      strlcpy(buf, mg.uri.ptr, urilen > sizeof(buf) ? sizeof(buf) : urilen);
      D_LOG("File '%s' requested", buf);

      mg_serve_fs(c, (void *)&mg, true);
    }
    else if (mg_http_match_uri(hm, "/reset"))
    {
      D_LOG("URI /reset");

      // add handle auth
      logMessagejson("Reset requested. Device will reboot in a few seconds...", WEBINTERFACE);
      mg_http_reply(c, 200, "", "HTTP OK");
      delay(200);
      shouldReboot = true;
    }
    else if (mg_http_match_uri(hm, "/edit"))
    {
      if (strncmp(hm->method.ptr, "GET", hm->method.len) == 0)
      {
        const char url[] = "/edit.html";
        const size_t len = sizeof(url) / sizeof(url[0]);
        struct mg_http_message mg;
        mg.uri.ptr = &url[0];
        mg.uri.len = len;

        mg_serve_fs(c, (void *)&mg, false);
      }
      else if (strncmp(hm->method.ptr, "PUT", hm->method.len) == 0)
      {
        mg_handleFileCreate(c, ev, ev_data, fn_data);
      }
      else if (strncmp(hm->method.ptr, "DELETE", hm->method.len) == 0)
      {
        mg_handleFileDelete(c, ev, ev_data, fn_data);
      }
    }
    else
    {
      D_LOG("URI else -> mg_serve_fs");

      mg_serve_fs(c, ev_data, false);
    }

    (void)fn_data;
  }
}

bool mg_packed_file_exists(void *ev_data)
{
  struct mg_http_message *hm = (struct mg_http_message *)ev_data;
  const struct packed_files_lst *ptr = packed_files_list;
  while (strcmp(ptr->name, "") != 0)
  {
    if (strncmp(ptr->name, hm->uri.ptr, hm->uri.len) == 0)
    {
      return true;
    }
    ptr++;
  }
  return false;
}

void mg_serve_fs(struct mg_connection *c, void *ev_data, bool download)
{
  if (c == nullptr)
    return;
  if (ev_data == nullptr)
    return;

  struct mg_http_message *hm = (struct mg_http_message *)ev_data;
  if (hm->uri.ptr == nullptr)
    return;

  char buf[64]{};
  int urilen = hm->uri.len + 1;
  strlcpy(buf, hm->uri.ptr, urilen > sizeof(buf) ? sizeof(buf) : urilen);
  D_LOG("File '%s' requested", buf);

  if (mg_packed_file_exists(ev_data))
  {
    D_LOG("File '%s' found on Packed file system", buf);
    struct mg_http_serve_opts opts;
    opts.root_dir = "/web_root";
    opts.fs = &mg_fs_packed;

    mg_http_serve_dir(c, (struct mg_http_message *)ev_data, &opts);
  }
  else if (mg_handleFileRead(c, ev_data, download))
  {

    // char buf[64]{};
    // strlcpy(buf, hm->uri.ptr, hm->uri.len + 1);
    D_LOG("File '%s' found on LittleFS file system", buf);
  }
  else
  {

    char buf[64]{};
    int len = hm->uri.len + 1;
    if (len > sizeof(buf))
      len = sizeof(buf);
    strlcpy(buf, hm->uri.ptr, len);
    D_LOG("File '%s' not found on any file system", buf);
  }
}

void mg_handleFileList(struct mg_connection *c, int ev, void *ev_data, void *fn_data)
{
  // add handle auth

  struct mg_http_message *hm = (struct mg_http_message *)ev_data;

  char dir[64]{};
  int get_var_res = mg_http_get_var(&hm->query, "dir", dir, sizeof(dir));

  if (get_var_res < 1)
  {
    mg_http_reply(c, 500, "", "%s", "BAD ARGS");
    return;
  }

  File root = ACTIVE_FS.open(dir);

  String output = "[";
  if (root.isDirectory())
  {
    File file = root.openNextFile();
    while (file)
    {
      if (output != "[")
      {
        output += ',';
      }
      output += "{\"type\":\"";
      output += (file.isDirectory()) ? "dir" : "file";
      output += "\",\"name\":\"";
      output += String(file.path()).substring(1);
      output += "\",\"size\":\"";
      output += String(file.size());
      output += "\"}";
      file = root.openNextFile();
    }
  }
  output += "]";

  root.close();

  mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s\n", output.c_str());
  output = String();
}

bool mg_handleFileRead(struct mg_connection *c, void *ev_data, bool download)
{
  // handle authentication
  D_LOG("mg_handleFileRead called");
  struct mg_http_message *hm = (struct mg_http_message *)ev_data;

  // if (hm->query.ptr != NULL)
  // {
  //   char buf[128]{};
  //   strlcpy(buf, hm->query.ptr, sizeof(buf));
  //   D_LOG("hm->query: %s, len: %d", buf, hm->query.len);
  // }

  // if (hm->body.ptr != NULL)
  // {
  //   char body[265]{};
  //   strlcpy(body, hm->body.ptr, sizeof(body));
  //   D_LOG("hm->body: %s, len: %d", body, hm->body.len);
  // }

  // if (hm->uri.ptr != NULL)
  // {
  //   char uri[128]{};
  //   strlcpy(uri, hm->uri.ptr, sizeof(uri));
  //   D_LOG("hm->uri: %s, len: %d", uri, hm->uri.len);
  // }
  // mg_http_reply(c, 200, "", "HTTP OK");
  // return false;

  char buf[64]{};
  strlcpy(buf, hm->uri.ptr, hm->uri.len + 1);
  String path = buf;

  String pathWithGz = path + ".gz";
  if (ACTIVE_FS.exists(pathWithGz) || ACTIVE_FS.exists(path))
  {
    D_LOG("file found ('%s')", path.c_str());

    if (ACTIVE_FS.exists(pathWithGz))
    {
      path += ".gz";
    }

    String contents;
    File file = ACTIVE_FS.open(path.c_str(), "r");
    if (!file)
    {
      D_LOG("mg_handleFileRead: file read error ('%s')", path.c_str());
      return false;
    }
    else
    {
      contents = file.readString();
      file.close();
      String contect_type = getContentType(download, path.c_str());
      contect_type += "\r\n";
      mg_http_reply(c, 200, contect_type.c_str(), "%s\n", contents.c_str());

      pathWithGz = String();
      path = String();
      return true;
    }
    pathWithGz = String();
    path = String();
    return false;
  }
  return false;
}

void mg_handleStatus(struct mg_connection *c, int ev, void *ev_data, void *fn_data)
{
  // handle auth
  D_LOG("mg_handleStatus called");

  size_t totalBytes = ACTIVE_FS.totalBytes();
  size_t usedBytes = ACTIVE_FS.usedBytes();

  String json;
  json.reserve(128);
  json = "{\"type\":\"Filesystem\", \"isOk\":";

  json += "\"true\", \"totalBytes\":\"";
  json += totalBytes;
  json += "\", \"usedBytes\":\"";
  json += usedBytes;
  json += "\"";

  json += ",\"unsupportedFiles\":\"\"}";

  mg_http_reply(c, 200, "Content-Type: application/json\r\n", "%s\n", json.c_str());
  json = String();
}

void mg_handleFileDelete(struct mg_connection *c, int ev, void *ev_data, void *fn_data)
{
}

void mg_handleFileCreate(struct mg_connection *c, int ev, void *ev_data, void *fn_data)
{

  // handle auth
  D_LOG("handleFileCreate called");

  struct mg_http_message *hm = (struct mg_http_message *)ev_data;

  if (hm->query.ptr != NULL)
  {
    char buf[128]{};
    strlcpy(buf, hm->query.ptr, sizeof(buf));
    D_LOG("hm->query: %s, len: %d", buf, hm->query.len);
  }

  if (hm->body.ptr != NULL)
  {
    char body[265]{};
    strlcpy(body, hm->body.ptr, sizeof(body));
    D_LOG("hm->body: %s, len: %d", body, hm->body.len);
  }

  if (hm->uri.ptr != NULL)
  {
    char uri[128]{};
    strlcpy(uri, hm->uri.ptr, sizeof(uri));
    D_LOG("hm->uri: %s, len: %d", uri, hm->uri.len);
  }

  if (hm->head.ptr != NULL)
  {
    char head[128]{};
    strlcpy(head, hm->head.ptr, sizeof(head));
    D_LOG("hm->head: %s, len: %d", head, hm->head.len);
  }
  char path[64]{};
  char src[64]{};
  // const char pathname[] = "path";
  // const char srcname[] = "src";

  struct mg_http_part part;
  size_t pos = 0;

  while ((pos = mg_http_next_multipart(hm->body, pos, &part)) > 0)
  {
    D_LOG("POS:%d", pos);
    if (part.name.ptr != NULL)
    {
      D_LOG("Chunk name:%s len:%d", part.name.ptr, (int)part.name.len);
    }
    if (part.filename.ptr != NULL)
    {
      D_LOG("filename:%s len:%d", part.filename.ptr, (int)part.filename.len);
    }
    if (part.body.ptr != NULL)
    {
      D_LOG("body:%s len:%d", part.body.ptr, (int)part.body.len);
    }
    if (part.name.ptr != NULL || part.filename.ptr != NULL || part.body.ptr != NULL)
    {
      D_LOG("");
    }
    if (part.name.ptr != NULL && part.body.ptr != NULL)
    {
      if (String(part.name.ptr).startsWith("path"))
      {
        strlcpy(path, part.body.ptr, part.body.len + 1);
        D_LOG("Path name found:[%s]", path);
      }
      if (String(part.name.ptr).startsWith("src"))
      {
        strlcpy(src, part.body.ptr, part.body.len + 1);
        D_LOG("Src name found:[%s]", src);
      }
    }
  }

  if (strncmp(path, "", sizeof(path)) == 0)
  {
    mg_http_reply(c, 400, "", "%s", "PATH ARG MISSING\n");
    return;
  }
  if (strncmp(path, "/", sizeof(path)) == 0)
  {
    mg_http_reply(c, 400, "", "%s", "BAD PATH\n");
    return;
  }

  String path_str = path;
  String src_str = src;

  if (src_str.isEmpty())
  {
    // No source specified: creation
    D_LOG("handleFileCreate: %s", path_str.c_str());
    if (path_str.endsWith("/"))
    {
      // Create a folder
      path_str.remove(path_str.length() - 1);
      if (!ACTIVE_FS.mkdir(path_str))
      {
        mg_http_reply(c, 500, "", "MKDIR FAILED");
        return;
      }
    }
    else
    {
      // Create a file
      File file = ACTIVE_FS.open(path_str, "w");
      if (file)
      {
        file.write(0);
        file.close();
      }
      else
      {
        mg_http_reply(c, 500, "", "CREATE FAILED");
        return;
      }
    }
    mg_http_reply(c, 200, "Content-Type: text/plain\r\n", path_str.c_str());
  }
  else
  {
    // Source specified: rename
    if (src_str == "/")
    {
      mg_http_reply(c, 400, "", "BAD SRC");
      return;
    }
    if (!ACTIVE_FS.exists(src_str))
    {
      mg_http_reply(c, 400, "", "SRC FILE NOT FOUND");
      return;
    }

    D_LOG("handleFileCreate: %s from %s\n", path_str.c_str(), src_str.c_str());
    if (path_str.endsWith("/"))
    {
      path_str.remove(path_str.length() - 1);
    }
    if (src_str.endsWith("/"))
    {
      src_str.remove(src_str.length() - 1);
    }
    if (!ACTIVE_FS.rename(src_str, path_str))
    {
      mg_http_reply(c, 500, "", "RENAME FAILED");
      return;
    }
    mg_http_reply(c, 200, "", "HTTP OK");
  }
}

// const char *mg_getContentType(bool download, const char *filename)
// {
//   if (download)
//     return "application/octet-stream\r\n";
//   else if (strstr(filename, ".htm"))
//     return "text/html\r\n";
//   else if (strstr(filename, ".html"))
//     return "text/html\r\n";
//   else if (strstr(filename, ".log"))
//     return "text/plain\r\n";
//   else if (strstr(filename, ".css"))
//     return "text/css\r\n";
//   else if (strstr(filename, ".sass"))
//     return "text/css\r\n";
//   else if (strstr(filename, ".js"))
//     return "application/javascript\r\n";
//   else if (strstr(filename, ".png"))
//     return "image/png\r\n";
//   else if (strstr(filename, ".svg"))
//     return "image/svg+xml\r\n";
//   else if (strstr(filename, ".gif"))
//     return "image/gif\r\n";
//   else if (strstr(filename, ".jpg"))
//     return "image/jpeg\r\n";
//   else if (strstr(filename, ".ico"))
//     return "image/x-icon\r\n";
//   else if (strstr(filename, ".xml"))
//     return "text/xml\r\n";
//   else if (strstr(filename, ".pdf"))
//     return "application/x-pdf\r\n";
//   else if (strstr(filename, ".zip"))
//     return "application/x-zip\r\n";
//   else if (strstr(filename, ".gz"))
//     return "application/x-gzip\r\n";
//   return "text/plain\r\n";
// }

#endif
