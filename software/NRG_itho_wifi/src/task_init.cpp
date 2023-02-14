#include "task_init.h"

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

  esp_task_wdt_init(60, true);
  I_LOG("Setup: done");

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

    if (digitalRead(fail_save_pin) == HIGH)
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

#endif

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
  hardware_rev_det = digitalRead(GPIO_NUM_25) << 5 | digitalRead(GPIO_NUM_26) << 4 | digitalRead(GPIO_NUM_32) << 3 | digitalRead(GPIO_NUM_33) << 2 | digitalRead(GPIO_NUM_21) << 1 | digitalRead(GPIO_NUM_22);

  // /*
  //  * 0x34 -> NON-CVE
  //  * 0x3F -> CVE i2c sniffer capable
  //  * 0x03 -> CVE not i2c sniffer capable
  //  */

  pinMode(GPIO_NUM_25, INPUT);
  pinMode(GPIO_NUM_26, INPUT);
  pinMode(GPIO_NUM_32, INPUT);
  pinMode(GPIO_NUM_33, INPUT);
  pinMode(GPIO_NUM_21, INPUT);
  pinMode(GPIO_NUM_22, INPUT);

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
  }

  if (hardware_rev_det == 0x3F || hardware_rev_det == 0x03) // CVE
  {
    boot_state_pin = GPIO_NUM_27;
    fail_save_pin = GPIO_NUM_14;
    itho_status_pin = GPIO_NUM_13;
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
    pinMode(status_pin, INPUT_PULLUP);
    pinMode(itho_status_pin, OUTPUT);
    digitalWrite(itho_status_pin, LOW);
  }
  else // NON-CVE
  {
    pinMode(status_pin, OUTPUT);
    digitalWrite(status_pin, LOW);
  }

#if defined(ENABLE_FAILSAVE_BOOT)
  pinMode(fail_save_pin, INPUT);
  // failSafeBoot();
#endif

  IthoInit = true;
}
