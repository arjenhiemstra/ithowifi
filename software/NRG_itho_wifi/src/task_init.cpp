#include "task_init.h"

// globals
bool TaskInitReady = false;

void TaskInit(void *pvParameters)
{
  configASSERT((uint32_t)pvParameters == 1UL);
  Ticker TaskTimeout;

  mutexLogTask = xSemaphoreCreateMutex();
  mutexJSONLog = xSemaphoreCreateMutex();
  mutexWSsend = xSemaphoreCreateMutex();

  hardwareInit();

  startTaskConfigAndLog();

  while (!TaskInitReady)
  {
    yield();
  }

  esp_task_wdt_init(40, true);
  logInput("Setup: done");

  vTaskDelete(NULL);
}

#if defined(ENABLE_FAILSAVE_BOOT)

void failSafeBoot()
{

  auto defaultWaitStart = millis() + 2000;
  auto ledblink = 0;

  while (millis() < defaultWaitStart)
  {
    yield();

    if (digitalRead(FAILSAVE_PIN) == HIGH)
    {

      ACTIVE_FS.begin(true);
      ACTIVE_FS.format();

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
      server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request)
                { request->send(200, "text/html", "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>"); });

      server.on(
          "/update", HTTP_POST, [](AsyncWebServerRequest *request)
          {
        shouldReboot = !Update.hasError();
        AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", shouldReboot ? "OK" : "FAIL");
        response->addHeader("Connection", "close");
        request->send(response); },
          [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
          {
            if (!index)
            {
              content_len = request->contentLength();
              D_LOG("Update Start: %s\n", filename.c_str());
              if (!Update.begin(UPDATE_SIZE_UNKNOWN))
              {
                Update.printError(Serial);
              }
            }
            if (!Update.hasError())
            {
              if (Update.write(data, len) != len)
              {

                Update.printError(Serial);
              }
            }
            if (final)
            {
              if (Update.end(true))
              {
              }
              else
              {
                Update.printError(Serial);
              }
              shouldReboot = true;
            }
          });
      server.begin();

      for (;;)
      {
        yield();
        if (shouldReboot)
        {
          ACTIVE_FS.end();
          delay(1000);
          esp_task_wdt_init(1, true);
          esp_task_wdt_add(NULL);
          while (true)
            ;
        }
        if (millis() - ledblink > 200)
        {
          ledblink = millis();
          if (digitalRead(WIFILED) == LOW)
          {
            digitalWrite(WIFILED, HIGH);
          }
          else
          {
            digitalWrite(WIFILED, LOW);
          }
        }
      }
    }

    if (millis() - ledblink > 50)
    {
      ledblink = millis();
      if (digitalRead(WIFILED) == LOW)
      {
        digitalWrite(WIFILED, HIGH);
      }
      else
      {
        digitalWrite(WIFILED, LOW);
      }
    }
  }
  digitalWrite(WIFILED, HIGH);
}

#endif

void hardwareInit()
{

  pinMode(WIFILED, OUTPUT);
  digitalWrite(WIFILED, HIGH);
#if defined(CVE)
  pinMode(STATUSPIN, INPUT_PULLUP);
  pinMode(ITHOSTATUS, OUTPUT);
  digitalWrite(ITHOSTATUS, LOW);
#elif defined(NON_CVE)
  pinMode(STATUSPIN, OUTPUT);
  digitalWrite(STATUSPIN, LOW);
#endif
#if defined(ENABLE_FAILSAVE_BOOT)
  pinMode(FAILSAVE_PIN, INPUT);
  failSafeBoot();
#endif
  pinMode(I2C_MASTER_SDA_IO, INPUT);
  pinMode(I2C_MASTER_SCL_IO, INPUT);

  IthoInit = true;
}
