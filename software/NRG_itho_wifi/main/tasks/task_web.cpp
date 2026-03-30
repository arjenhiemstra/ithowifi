#include "tasks/task_web.h"
#include "api/OpenAPI.h"
#include "api/WebAPIv2Rest.h"

#include "webroot/controls_js_gz.h"
#include "webroot/edit_html_gz.h"
#include "webroot/favicon_png_gz.h"
#include "webroot/index_html_gz.h"
#include "webroot/pure_min_css_gz.h"

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
bool updateMQTTmodeStatus = false;
bool webauth_ok = false;
char modestate[32];

AsyncWebServer server(80);

// locals
StaticTask_t xTaskWebBuffer;
StackType_t xTaskWebStack[STACK_SIZE_MEDIUM];

unsigned long lastSysMessage = 0;
unsigned long previousUpdate = 0;
bool onOTA = false;
bool TaskWebStarted = false;

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

    TaskTimeout.once_ms(TASK_WEB_TIMEOUT_MS, []()
                        { W_LOG("SYS: warning - Task Web timed out!"); });

    execWebTasks();

    TaskWebHWmark = uxTaskGetStackHighWaterMark(NULL);
    vTaskDelay(25 / portTICK_PERIOD_MS);
  }

  // else delete task
  vTaskDelete(NULL);
}

void execWebTasks()
{
  ArduinoOTA.handle();

  ws.cleanupClients();

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

  server.on("/edit.html", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    if (systemConfig.syssec_edit) {
      if (!request->authenticate(systemConfig.sys_username, systemConfig.sys_password))
      return request->requestAuthentication();
    }
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", edit_html_gz, edit_html_gz_len, nullptr);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response); });

  server.on("/edit.html", HTTP_PUT, handleFileCreate);
  server.on("/edit.html", HTTP_DELETE, handleFileDelete);

  // run handleUpload function when any file is uploaded
  server.on(
      "/edit.html", HTTP_POST, [](AsyncWebServerRequest *request)
      { request->send(200); },
      handleUpload);

  server.on("/list", HTTP_GET, handleFileList);
  server.on("/status", HTTP_GET, handleStatus);

  // server.on("/crash", HTTP_GET, handleCoreCrash);
  server.on("/getcoredump", HTTP_GET, handleCoredumpDownload);

  // css_code
  server.on("/pure-min.css", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/css", pure_min_css_gz, pure_min_css_gz_len, nullptr);
    response->addHeader("Content-Encoding", "gzip");
    request->send(response); });

  // javascript files
  server.on("/controls.js", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    AsyncWebServerResponse *response = request->beginResponse(200, "application/javascript", controls_js_gz, controls_js_gz_len, nullptr);
    response->addHeader("Server", "Itho WiFi Web Server");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response); });

  server.on("/general.js", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    AsyncResponseStream *response = request->beginResponseStream("application/javascript");
    response->addHeader("Server", "Itho WiFi Web Server");

    int wizardStep = 0;
    if (ACTIVE_FS.exists("/wizard_state")) {
      File f = ACTIVE_FS.open("/wizard_state", "r");
      if (f) { wizardStep = f.parseInt(); f.close(); }
    }

    response->print("var on_ap = false; var wizard_step = ");
    response->print(wizardStep);
    response->print("; var hostname = '");
    response->print(hostName());
    response->print("'; var fw_version = '");
    response->print(fw_version);
    response->print("'; var hw_revision = '");
    response->print(hardwareManager.hw_revision);
    response->print("'; var itho_rf_standalone = '");
    response->print(systemConfig.itho_rf_standalone);
    response->print("'; var itho_pwm2i2c = '");
    response->print(systemConfig.itho_pwm2i2c);
    response->print("'; document.addEventListener('DOMContentLoaded', function() { var h = $id('headingindex'); h.textContent = hostname; h.href = 'http://' + hostname + '.local'; var m = $id('main'); m.insertAdjacentHTML('beforeend', ");
    response->print(wizardStep > 0 ? "html_wizard" : "html_index");
    response->print("); execScripts(m); });");

    request->send(response); })
      .setFilter(ON_STA_FILTER);

  server.on("/general.js", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    AsyncResponseStream *response = request->beginResponseStream("application/javascript");
    response->addHeader("Server", "Itho WiFi Web Server");

    bool isFirstBoot = (strlen(wifiConfig.ssid) == 0);
    int wizardStep = 0;
    if (ACTIVE_FS.exists("/wizard_state")) {
      File f = ACTIVE_FS.open("/wizard_state", "r");
      if (f) { wizardStep = f.parseInt(); f.close(); }
    }
    bool showWizard = isFirstBoot || wizardStep > 0;

    response->print("var on_ap = true; var first_boot = ");
    response->print(isFirstBoot ? "true" : "false");
    response->print("; var wizard_step = ");
    response->print(wizardStep);
    response->print("; var hostname = '");
    response->print(hostName());
    response->print("'; var fw_version = '");
    response->print(fw_version);
    response->print("'; var hw_revision = '");
    response->print(hardwareManager.hw_revision);
    response->print("'; var itho_rf_standalone = '");
    response->print(systemConfig.itho_rf_standalone);
    response->print("'; var itho_pwm2i2c = '");
    response->print(systemConfig.itho_pwm2i2c);
    response->print("'; document.addEventListener('DOMContentLoaded', function() { var h = $id('headingindex'); h.textContent = hostname; h.href = 'http://' + hostname + '.local'; var m = $id('main'); m.insertAdjacentHTML('beforeend', ");
    response->print(showWizard ? "html_wizard" : "html_wifisetup");
    response->print("); execScripts(m); });");

    request->send(response); })
      .setFilter(ON_AP_FILTER);

  server.on("/favicon.png", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    AsyncWebServerResponse *response = request->beginResponse(200, "image/png", favicon_png_gz, favicon_png_gz_len, nullptr);
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
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", index_html_gz, index_html_gz_len, nullptr);
    response->addHeader("Server", "Itho WiFi Web Server");
    response->addHeader("Content-Encoding", "gzip");
    request->send(response); });
  server.on("/api.html", HTTP_GET, handleAPI);
  server.on("/api/openapi.json", HTTP_GET, handleOpenAPI);
  registerRestAPIv2Routes(server);

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
          D_LOG("OTA: Begin OTA update");
          onOTA = true;
          dontReconnectMQTT = true;
          mqttClient.disconnect();
          i2cSnifferUnload();

          content_len = request->contentLength();

          N_LOG("OTA: firmware update: %s", filename.c_str());
          delay(1000);

          if (xTaskCC1101Handle != NULL)
          {
            disableRF_ISR();
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
            D_LOG("OTA: Update Success: %uB", index + len);
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
  N_LOG("NET: webserver started");
}
