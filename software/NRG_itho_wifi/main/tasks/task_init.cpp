#include "tasks/task_init.h"

#define TWDT_TIMEOUT_MS 60000

// globals
bool TaskInitReady = false;

void TaskInit(void *pvParameters)
{
  configASSERT((uint32_t)pvParameters == 1UL);
  Ticker TaskTimeout;

  mutexJSONLog = xSemaphoreCreateMutex();
  mutexWSsend = xSemaphoreCreateMutex();

  hardwareInit();

  if (hardwareManager.isSnifferCapable())
  {
    i2cSnifferInit(false);
  }

  startTaskConfigAndLog();

  while (!TaskInitReady)
  {
    yield();
  }

#include "esp_idf_version.h"

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
  const esp_task_wdt_config_t twdt_config = {
      .timeout_ms = TWDT_TIMEOUT_MS,
      .idle_core_mask = (1 << portNUM_PROCESSORS) - 1, // Bitmask of all cores
      .trigger_panic = true,
  };
  esp_task_wdt_init(&twdt_config);
#else
  esp_task_wdt_init(60, true);
#endif

  I_LOG("SYS: setup done");
  vTaskDelete(NULL);
}

void failSafeBoot()
{

  auto defaultWaitStart = millis() + 2000;
  auto ledblink = 0;

  while (millis() < defaultWaitStart)
  {
    yield();

    if (digitalRead(hardwareManager.fail_save_pin) == HIGH)
    {

      ACTIVE_FS.begin(true);
      ACTIVE_FS.format();

      IPAddress apIP(192, 168, 4, 1);
      IPAddress netMsk(255, 255, 255, 0);

      WiFi.persistent(false);
      WiFi.disconnect(); //  this alone is not enough to stop the autoconnecter
      WiFi.mode(WIFI_AP_STA);
      delay(2000);

      WiFi.softAP(hostName(), wifiConfig.appasswd);
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
              D_LOG("OTA: Update Start: %s", filename.c_str());
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
          esp_restart();
        }
        if (millis() - ledblink > 200)
        {
          ledblink = millis();
          if (digitalRead(hardwareManager.wifi_led_pin) == LOW)
          {
            digitalWrite(hardwareManager.wifi_led_pin, HIGH);
          }
          else
          {
            digitalWrite(hardwareManager.wifi_led_pin, LOW);
          }
        }
      }
    }

    if (millis() - ledblink > 50)
    {
      ledblink = millis();
      if (digitalRead(hardwareManager.wifi_led_pin) == LOW)
      {
        digitalWrite(hardwareManager.wifi_led_pin, HIGH);
      }
      else
      {
        digitalWrite(hardwareManager.wifi_led_pin, LOW);
      }
    }
  }
  digitalWrite(hardwareManager.wifi_led_pin, HIGH);
}

void hardwareInit()
{
  // Hardware detection now handled by HardwareManager
  hardwareManager.detect();

  // Initialize I2C sniffer if hardware supports it
  if (hardwareManager.isSnifferCapable())
  {
    i2cSnifferInit(false);
  }

  // Fail-safe boot check
  failSafeBoot();

  // Set initialization flag
  IthoInit = true;
}
