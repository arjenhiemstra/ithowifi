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

  if (i2c_sniffer_capable)
  {
    i2c_sniffer_init(false);
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

  I_LOG("Setup: done");
  forceFWcheck = true;
  vTaskDelete(NULL);
}

void failSafeBoot()
{

  auto defaultWaitStart = millis() + 2000;
  auto ledblink = 0;

  while (millis() < defaultWaitStart)
  {
    yield();

    if (digitalRead(fail_save_pin) == HIGH)
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
#if defined MG_ENABLE_PACKED_FS && MG_ENABLE_PACKED_FS == 1
// implement mongoose based ota
#else
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
              D_LOG("Update Start: %s", filename.c_str());
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
#endif
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
          if (digitalRead(wifi_led_pin) == LOW)
          {
            digitalWrite(wifi_led_pin, HIGH);
          }
          else
          {
            digitalWrite(wifi_led_pin, LOW);
          }
        }
      }
    }

    if (millis() - ledblink > 50)
    {
      ledblink = millis();
      if (digitalRead(wifi_led_pin) == LOW)
      {
        digitalWrite(wifi_led_pin, HIGH);
      }
      else
      {
        digitalWrite(wifi_led_pin, LOW);
      }
    }
  }
  digitalWrite(wifi_led_pin, HIGH);
}

void hardwareInit()
{
  delay(1000);

  // test pin connections te determine which hardware (and revision if applicable) we are dealing with
  pinMode(GPIO_NUM_25, INPUT_PULLDOWN);
  pinMode(GPIO_NUM_26, INPUT_PULLDOWN);
  pinMode(GPIO_NUM_32, INPUT_PULLDOWN);
  pinMode(GPIO_NUM_33, INPUT_PULLDOWN);
  pinMode(GPIO_NUM_21, INPUT_PULLDOWN);
  pinMode(GPIO_NUM_22, INPUT_PULLDOWN);

  delay(50);

  pinMode(GPIO_NUM_25, INPUT);
  pinMode(GPIO_NUM_26, INPUT);
  pinMode(GPIO_NUM_32, INPUT);
  pinMode(GPIO_NUM_33, INPUT);
  pinMode(GPIO_NUM_21, INPUT);
  pinMode(GPIO_NUM_22, INPUT);

  delay(50);

  hardware_rev_det = digitalRead(GPIO_NUM_25) << 5 | digitalRead(GPIO_NUM_26) << 4 | digitalRead(GPIO_NUM_32) << 3 | digitalRead(GPIO_NUM_33) << 2 | digitalRead(GPIO_NUM_21) << 1 | digitalRead(GPIO_NUM_22);

  // /*
  //  * 0x34 -> NON-CVE
  //  * 0x3F -> CVE i2c sniffer capable
  //  * 0x03 -> CVE not i2c sniffer capable
  //  */

  if (hardware_rev_det == 0x3F || hardware_rev_det == 0x34)
  {
    i2c_sniffer_capable = true;
    if (hardware_rev_det == 0x3F) // CVE i2c sniffer capable
    {
      i2c_master_setpins(GPIO_NUM_26, GPIO_NUM_32);
      i2c_slave_setpins(GPIO_NUM_26, GPIO_NUM_32);
      i2c_sniffer_setpins(GPIO_NUM_21, GPIO_NUM_22);
    }
    else
    { // NON-CVE
      pinMode(GPIO_NUM_27, INPUT);
      pinMode(GPIO_NUM_13, INPUT);
      pinMode(GPIO_NUM_14, INPUT);
      i2c_master_setpins(GPIO_NUM_27, GPIO_NUM_26);
      i2c_slave_setpins(GPIO_NUM_27, GPIO_NUM_26);
      i2c_sniffer_setpins(GPIO_NUM_14, GPIO_NUM_25);
    }
  }
  else // CVE i2c not sniffer capable
  {
    i2c_master_setpins(GPIO_NUM_21, GPIO_NUM_22);
    i2c_slave_setpins(GPIO_NUM_21, GPIO_NUM_22);
  }

  if (hardware_rev_det == 0x3F || hardware_rev_det == 0x03) // CVE
  {
    boot_state_pin = GPIO_NUM_27;
    fail_save_pin = GPIO_NUM_14;
    status_pin = GPIO_NUM_13;
    hw_revision = cve2;
  }
  else // NON-CVE
  {
    fail_save_pin = GPIO_NUM_32;
    hw_revision = non_cve1;
  }

  pinMode(wifi_led_pin, OUTPUT);
  digitalWrite(wifi_led_pin, HIGH);

  if (hardware_rev_det == 0x3F || hardware_rev_det == 0x03) // CVE
  {
    pinMode(GPIO_NUM_16, INPUT_PULLUP);
  }
  pinMode(status_pin, OUTPUT);
  digitalWrite(status_pin, LOW);
  pinMode(fail_save_pin, INPUT);
  failSafeBoot();

  IthoInit = true;
}
