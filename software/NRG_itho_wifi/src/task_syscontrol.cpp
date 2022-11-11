

#include "task_syscontrol.h"

#define TASK_SYS_CONTROL_PRIO 5

// globals
uint32_t TaskSysControlHWmark = 0;
DNSServer dnsServer;
bool SHT3x_original = false;
bool SHT3x_alternative = false;
bool runscan = false;
bool dontSaveConfig = false;
bool saveSystemConfigflag = false;
bool saveWifiConfigflag = false;
bool resetWifiConfigflag = false;
bool resetSystemConfigflag = false;
bool clearQueue = false;
bool shouldReboot = false;
int8_t ithoInitResult = 0;
bool IthoInit = false;
bool wifiModeAP = false;
bool reset_sht_sensor = false;

// locals
StaticTask_t xTaskSysControlBuffer;
StackType_t xTaskSysControlStack[STACK_SIZE + STACK_SIZE_SMALL];
TaskHandle_t xTaskSysControlHandle = NULL;
SemaphoreHandle_t mutexI2Ctask;
unsigned long lastI2CinitRequest = 0;
bool ithoInitResultLogEntry = true;
Ticker scan;
Ticker getSettingsHack;
unsigned long lastWIFIReconnectAttempt = 0;
unsigned long APmodeTimeout = 0;
unsigned long wifiLedUpdate = 0;
unsigned long SHT3x_readout = 0;
unsigned long query2401tim = 0;
unsigned long mqttUpdatetim = 0;
unsigned long lastVersionCheck = 0;
bool i2cStartCommands = false;
bool joinSend = false;

SHTSensor sht_org(SHTSensor::SHT3X);
SHTSensor sht_alt(SHTSensor::SHT3X_ALT);

void startTaskSysControl()
{
  xTaskSysControlHandle = xTaskCreateStaticPinnedToCore(
      TaskSysControl,
      "TaskSysControl",
      STACK_SIZE + STACK_SIZE_SMALL,
      (void *)1,
      TASK_SYS_CONTROL_PRIO,
      xTaskSysControlStack,
      &xTaskSysControlBuffer,
      CONFIG_ARDUINO_RUNNING_CORE);
}

void TaskSysControl(void *pvParameters)
{
  configASSERT((uint32_t)pvParameters == 1UL);
  Ticker TaskTimeout;
  Ticker queueUpdater;

  mutexI2Ctask = xSemaphoreCreateMutex();

  wifiInit();

  init_vRemote();

  initSensor();

  startTaskCC1101();

  esp_task_wdt_add(NULL);

  queueUpdater.attach_ms(QUEUE_UPDATE_MS, update_queue);

  for (;;)
  {
    yield();
    esp_task_wdt_reset();

    TaskTimeout.once_ms(35000, []()
                        { logInput("Warning: Task SysControl timed out!"); });

    execSystemControlTasks();

    if (shouldReboot)
    {
      TaskTimeout.detach();
      logInput("Reboot requested");
      if (!dontSaveConfig)
      {
        saveSystemConfig();
      }
      delay(1000);
      ACTIVE_FS.end();
      esp_task_wdt_init(1, true);
      esp_task_wdt_add(NULL);
      while (true)
        ;
    }

    TaskSysControlHWmark = uxTaskGetStackHighWaterMark(NULL);
    vTaskDelay(25 / portTICK_PERIOD_MS);
  }

  // else delete task
  vTaskDelete(NULL);
}

void execSystemControlTasks()
{

  if (IthoInit && millis() > 250)
  {
    IthoInit = ithoInitCheck();
  }

  if (!i2cStartCommands && millis() > 15000 && (millis() - lastI2CinitRequest > 5000))
  {
    lastI2CinitRequest = millis();
    if (xSemaphoreTake(mutexI2Ctask, (TickType_t)500 / portTICK_PERIOD_MS) == pdTRUE)
    {
      sendQueryDevicetype(false);
      xSemaphoreGive(mutexI2Ctask);
    }
    if (currentItho_fwversion() > 0)
    {
      char logBuff[LOG_BUF_SIZE]{};
      sprintf(logBuff, "I2C init: QueryDevicetype - fw:%d hw:%d", currentItho_fwversion(), currentIthoDeviceID());
      logInput(logBuff);

      ithoInitResult = 1;
      i2cStartCommands = true;
#if defined(CVE)
      digitalWrite(ITHOSTATUS, HIGH);
#elif defined(NON_CVE)
      digitalWrite(STATUSPIN, HIGH);
#endif

      if (systemConfig.syssht30 > 0)
      {
        if (currentIthoDeviceID() == 0x1B)
        {

          if (xSemaphoreTake(mutexI2Ctask, (TickType_t)500 / portTICK_PERIOD_MS) == pdTRUE)
          {
            i2c_result_updateweb = false;

            index2410 = 0;

            if (systemConfig.syssht30 == 1)
            {
              /*
                 switch itho hum setting off on every boot because
                 hum sensor setting gets restored in itho firmware after power cycle
              */
              value2410 = 0;
            }

            else if (systemConfig.syssht30 == 2)
            {
              /*
                 if value == 2 setting changed from 1 -> 0
                 then restore itho hum setting back to "on"
              */
              value2410 = 1;
            }
            if (currentItho_fwversion() == 25)
            {
              index2410 = 63;
            }
            else if (currentItho_fwversion() == 26 || currentItho_fwversion() == 27)
            {
              index2410 = 71;
            }
            if (index2410 > 0)
            {
              sendQuery2410(i2c_result_updateweb);
              setSetting2410(i2c_result_updateweb);
              char logBuff[LOG_BUF_SIZE]{};
              sprintf(logBuff, "I2C init: set hum sensor in itho firmware to: %s", value2410 ? "on" : "off");
              logInput(logBuff);
            }
            xSemaphoreGive(mutexI2Ctask);
          }
        }
        if (systemConfig.syssht30 == 2)
        {
          systemConfig.syssht30 = 0;
          saveSystemConfig();
        }
      }
      if (xSemaphoreTake(mutexI2Ctask, (TickType_t)500 / portTICK_PERIOD_MS) == pdTRUE)
      {
        sendQueryStatusFormat(false);
        char logBuff[LOG_BUF_SIZE]{};
        sprintf(logBuff, "I2C init: QueryStatusFormat - items:%d", ithoStatus.size());
        logInput(logBuff);
        xSemaphoreGive(mutexI2Ctask);
      }
      if (xSemaphoreTake(mutexI2Ctask, (TickType_t)500 / portTICK_PERIOD_MS) == pdTRUE)
      {
        sendQueryStatus(false);
        logInput("I2C init: QueryStatus");
        xSemaphoreGive(mutexI2Ctask);
      }

      sendHomeAssistantDiscovery = true;
    }
    else
    {
      if (ithoInitResultLogEntry)
      {
        ithoInitResult = -1;
        ithoInitResultLogEntry = false;
        logInput("I2C init: QueryDevicetype - failed");
      }
    }
  }

  if (systemConfig.itho_sendjoin > 0 && !joinSend && ithoInitResult == 1)
  {

    if (xSemaphoreTake(mutexI2Ctask, (TickType_t)500 / portTICK_PERIOD_MS) == pdTRUE)
    {
      joinSend = true;
      sendRemoteCmd(0, IthoJoin, virtualRemotes);
      xSemaphoreGive(mutexI2Ctask);
      logInput("I2C init: Virtual remote join command send");

      if (systemConfig.itho_sendjoin == 1)
      {
        systemConfig.itho_sendjoin = 0;
        saveSystemConfig();
      }
    }
  }

  // Itho queue
  if (clearQueue)
  {
    clearQueue = false;
    ithoQueue.clear_queue();
  }
  if (ithoQueue.ithoSpeedUpdated)
  {
    ithoQueue.ithoSpeedUpdated = false;
    uint16_t speed = ithoQueue.get_itho_speed();
    char buf[32]{};
    sprintf(buf, "speed:%d", speed);

    //#if defined (CVE)
    if (writeIthoVal(speed, &ithoCurrentVal, &updateIthoMQTT))
    {
      if (lastCmd.source == nullptr)
        logLastCommand(buf, "ithoQueue");
      sysStatReq = true;
    }
    else
    {
      logLastCommand(buf, "ithoQueue_error");
    }
    //#endif
  }
  // System control tasks
  if ((WiFi.status() != WL_CONNECTED) && !wifiModeAP)
  {
    if (millis() - lastWIFIReconnectAttempt > 60000)
    {
      logInput("Attempt to reconnect WiFi");
      lastWIFIReconnectAttempt = millis();
      // Attempt to reconnect
      if (connectWiFiSTA())
      {
        logInput("Reconnect WiFi successful");
        lastWIFIReconnectAttempt = 0;
      }
      else
      {
        logInput("Reconnect WiFi failed!");
      }
    }
  }
  if (runscan)
  {
    runscan = false;
    scan.once_ms(10, wifiScan);
  }

  if (wifiModeAP)
  {
    if (millis() - APmodeTimeout > 900000)
    { // reboot after 15 min in AP mode
      shouldReboot = true;
    }
    dnsServer.processNextRequest();

    if (millis() - wifiLedUpdate >= 500)
    {
      wifiLedUpdate = millis();
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

  if (!(currentItho_fwversion() > 0) && millis() - lastVersionCheck > 60000)
  {
    lastVersionCheck = millis();
    if (xSemaphoreTake(mutexI2Ctask, (TickType_t)500 / portTICK_PERIOD_MS) == pdTRUE)
    {
      sendQueryDevicetype(false);
      xSemaphoreGive(mutexI2Ctask);
    }
  }
  // request itho status every systemConfig.itho_updatefreq sec.
  if (millis() - query2401tim >= systemConfig.itho_updatefreq * 1000UL && i2cStartCommands)
  {
    query2401tim = millis();
    if (xSemaphoreTake(mutexI2Ctask, (TickType_t)500 / portTICK_PERIOD_MS) == pdTRUE)
    {
      sendQueryStatus(false);
      xSemaphoreGive(mutexI2Ctask);
    }
    if (xSemaphoreTake(mutexI2Ctask, (TickType_t)500 / portTICK_PERIOD_MS) == pdTRUE)
    {
      sendQuery31DA(false);
      xSemaphoreGive(mutexI2Ctask);
    }
    if (xSemaphoreTake(mutexI2Ctask, (TickType_t)500 / portTICK_PERIOD_MS) == pdTRUE)
    {
      sendQuery31D9(false);
      xSemaphoreGive(mutexI2Ctask);
    }
    updateMQTTihtoStatus = true;
  }
  if (ithoInitResult == -1)
  {
    if (millis() - mqttUpdatetim >= systemConfig.itho_updatefreq * 1000UL)
    {
      mqttUpdatetim = millis();
      updateMQTTihtoStatus = true;
    }
  }
  if (get2410)
  {
    if (xSemaphoreTake(mutexI2Ctask, (TickType_t)500 / portTICK_PERIOD_MS) == pdTRUE)
    {
      get2410 = false;
      resultPtr2410 = sendQuery2410(i2c_result_updateweb);

      xSemaphoreGive(mutexI2Ctask);
    }
  }
  if (set2410)
  {
    if (xSemaphoreTake(mutexI2Ctask, (TickType_t)500 / portTICK_PERIOD_MS) == pdTRUE)
    {
      setSetting2410(i2c_result_updateweb);
      set2410 = false;

      xSemaphoreGive(mutexI2Ctask);
    }
    getSettingsHack.once_ms(1, []()
                            { getSetting(index2410, true, false, false); });
  }

  if (systemConfig.syssht30 == 1)
  {
    if (millis() - SHT3x_readout >= systemConfig.itho_updatefreq * 1000UL && (SHT3x_original || SHT3x_alternative))
    {
      SHT3x_readout = millis();

      if (SHT3x_original || SHT3x_alternative)
      {
        if (xSemaphoreTake(mutexI2Ctask, (TickType_t)500 / portTICK_PERIOD_MS) == pdTRUE)
        {

          if (SHT3x_original)
          {
            if (reset_sht_sensor)
            {
              reset_sht_sensor = false;
              bool reset_res = sht_org.softReset();
              if (reset_res)
              {
                ithoInitResult = 0;
                i2cStartCommands = false;
                sysStatReq = true;
              }
              char buf[32]{};
              sprintf(buf, "SHT3x sensor reset: %s", reset_res ? "Success" : "Failed");
              logInput(buf);
              sprintf(buf, "%s", reset_res ? "OK" : "NOK");
              jsonSysmessage("i2c_sht_reset", buf);
            }
            else if (sht_org.readSample())
            {
              ithoHum = sht_org.getHumidity();
              ithoTemp = sht_org.getTemperature();
            }
          }
          if (SHT3x_alternative)
          {
            if (reset_sht_sensor)
            {
              reset_sht_sensor = false;
              bool reset_res = sht_alt.softReset();
              if (reset_res)
              {
                ithoInitResult = 0;
                i2cStartCommands = false;
                sysStatReq = true;
              }
              char buf[32]{};
              sprintf(buf, "SHT3x sensor reset: %s", reset_res ? "Success" : "Failed");
              logInput(buf);
              sprintf(buf, "%s", reset_res ? "OK" : "NOK");
              jsonSysmessage("i2c_sht_reset", buf);
            }
            else if (sht_alt.readSample())
            {
              ithoHum = sht_alt.getHumidity();
              ithoTemp = sht_alt.getTemperature();
            }
          }
          xSemaphoreGive(mutexI2Ctask);
        }
      }
    }
  }
  if (reset_sht_sensor && (!SHT3x_alternative && !SHT3x_original))
  {
    if (xSemaphoreTake(mutexI2Ctask, (TickType_t)500 / portTICK_PERIOD_MS) == pdTRUE)
    {
      reset_sht_sensor = false;
      trigger_sht_sensor_reset();
      ithoInitResult = 0;
      i2cStartCommands = false;
      sysStatReq = true;
      logInput("SHT3x sensor reset: executed");
      jsonSysmessage("i2c_sht_reset", "Done");
      xSemaphoreGive(mutexI2Ctask);
    }
    else
    {
      logInput("SHT3x sensor reset: failed (mutex)");
      jsonSysmessage("i2c_sht_reset", "Failed (mutex)");
    }
  }
}

void wifiInit()
{
  if (!loadWifiConfig())
  {
    logInput("Setup: Wifi config load failed");
    setupWiFiAP();
  }
  else if (!connectWiFiSTA(true))
  {
    logInput("Setup: Wifi connect STA failed");
    setupWiFiAP();
  }
  configTime(0, 0, wifiConfig.ntpserver);

  WiFi.scanDelete();
  if (WiFi.scanComplete() == -2)
  {
    WiFi.scanNetworks(true);
  }
  if (!wifiModeAP)
  {
    logWifiInfo();
  }
  else
  {
    logInput("Setup: AP mode active");
  }
}

void setupWiFiAP()
{

  /* Soft AP network parameters */
  IPAddress apIP(192, 168, 4, 1);
  IPAddress netMsk(255, 255, 255, 0);

  WiFi.persistent(false); // Do not use SDK storage of SSID/WPA parameters
  esp_wifi_set_storage(WIFI_STORAGE_RAM);

  WiFi.disconnect(true);
  WiFi.setAutoReconnect(false);
  delay(200);
  WiFi.mode(WIFI_AP);

  esp_wifi_set_ps(WIFI_PS_NONE);

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

bool connectWiFiSTA(bool restore)
{

  wifiModeAP = false;
  D_LOG("Connecting to wireless network...\n");

  // Do not use SDK storage of SSID/WPA parameters
  WiFi.persistent(false);

#ifdef ESPRESSIF32_3_5_0_WIFI
  //
  // Do not use flash storage for wifi settings
  esp_err_t wifi_set_storage = esp_wifi_set_storage(WIFI_STORAGE_RAM);
  D_LOG("esp_wifi_set_storage: %s\n", esp_err_to_name(wifi_set_storage));
  // Disconnect any existing connections and clear
  if (!WiFi.disconnect(true))
    D_LOG("Unable to set wifi disconnect\n");

  // WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
  //  Set hostname
  bool setHostname_result = WiFi.setHostname(hostName());
  D_LOG("WiFi.setHostname: %s\n", setHostname_result ? "OK" : "NOK");

  // No AutoReconnect
  if (!WiFi.setAutoReconnect(false))
    D_LOG("Unable to set auto reconnect\n");
  delay(200);
  // Set correct mode
  if (!WiFi.mode(WIFI_STA))
    D_LOG("Unable to set WiFi mode\n");

  // No power saving
  esp_err_t wifi_set_ps = esp_wifi_set_ps(WIFI_PS_NONE);
  D_LOG("esp_wifi_set_ps: %s\n", esp_err_to_name(wifi_set_ps));
  //
#else
  // Set wifi mode to STA for next step (clear config)
  if (!WiFi.mode(WIFI_STA))
    D_LOG("Unable to set WiFi mode to STA\n");

  // Clear any saved wifi config
  if (restore)
  {
    esp_err_t wifi_restore = esp_wifi_restore();
    D_LOG("esp_wifi_restore: %s\n", esp_err_to_name(wifi_restore));
  }
  // Disconnect any existing connections and clear
  if (!WiFi.disconnect(true, true))
    D_LOG("Unable to set wifi disconnect\n");

  // Reset wifi mode to NULL
  if (!WiFi.mode(WIFI_MODE_NULL))
    D_LOG("Unable to set WiFi mode to NULL\n");

  // Set hostname
  bool setHostname_result = WiFi.setHostname(hostName());
  D_LOG("WiFi.setHostname: %s\n", setHostname_result ? "OK" : "NOK");

  // No AutoReconnect
  if (!WiFi.setAutoReconnect(false))
    D_LOG("Unable to set auto reconnect\n");

  delay(200);

  // Begin init of actual wifi connection

  // Set correct mode
  if (!WiFi.mode(WIFI_STA))
    D_LOG("Unable to set WiFi mode\n");

  esp_err_t esp_wifi_set_max_tx_power(int8_t power);
  esp_err_t wifi_set_max_tx_power = esp_wifi_set_max_tx_power(78);
  D_LOG("esp_wifi_set_max_tx_power: %s\n", esp_err_to_name(wifi_set_max_tx_power));

  int8_t wifi_power_level = -1;
  esp_err_t wifi_get_max_tx_power = esp_wifi_get_max_tx_power(&wifi_power_level);
  D_LOG("esp_wifi_get_max_tx_power: %s - level:%d\n", esp_err_to_name(wifi_get_max_tx_power), wifi_power_level);

  // Do not use flash storage for wifi settings
  esp_err_t wifi_set_storage = esp_wifi_set_storage(WIFI_STORAGE_RAM);
  D_LOG("esp_wifi_set_storage: %s\n", esp_err_to_name(wifi_set_storage));

  // No power saving
  esp_err_t wifi_set_ps = esp_wifi_set_ps(WIFI_PS_NONE);
  D_LOG("esp_wifi_set_ps: %s\n", esp_err_to_name(wifi_set_ps));

  // Switch from defualt WIFI_FAST_SCAN ScanMethod to WIFI_ALL_CHANNEL_SCAN. Also set sortMethod to WIFI_CONNECT_AP_BY_SIGNAL just to be sure.
  // WIFI_FAST_SCAN will connect to the first SSID match found causing connection issues when multiple AP active with the same SSID name
  WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);
  WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);

#endif

  if (strcmp(wifiConfig.dhcp, "off") == 0)
  {
    set_static_ip_config();
  }

  delay(2000);

  wl_status_t wifi_begin = WiFi.begin(wifiConfig.ssid, wifiConfig.passwd);

  auto timeoutmillis = millis() + 30000;
  wl_status_t status = WiFi.status();

  while (millis() < timeoutmillis)
  {

    esp_task_wdt_reset();

    status = WiFi.status();
    if (status == WL_CONNECTED)
    {
      digitalWrite(WIFILED, LOW);
      return true;
    }
    // else if (status != WL_DISCONNECTED && status != WL_IDLE_STATUS) // fix for issue #108
    // {
    //   char wifiBuff[64]{};
    //   sprintf(wifiBuff, "WiFi: status NOK [%s], reinitializing...", wifiConfig.(status));
    //   logInput(wifiBuff);
    //   strlcpy(wifiBuff, "", sizeof(wifiBuff));

    //   WiFi.disconnect();
    //   WiFi.mode(WIFI_OFF);
    //   if (strcmp(wifiConfig.dhcp, "off") == 0)
    //   {
    //     set_static_ip_config();
    //   }
    //   WiFi.mode(WIFI_STA);

    //   delay(1000);

    //   wifi_begin = WiFi.begin(wifiConfig.ssid, wifiConfig.passwd);
    //   delay(2000);
    // }
    if (digitalRead(WIFILED) == LOW)
    {
      digitalWrite(WIFILED, HIGH);
    }
    else
    {
      digitalWrite(WIFILED, LOW);
    }

    delay(100);
  }

  digitalWrite(WIFILED, HIGH);

  char buf[64]{};
  sprintf(buf, "Setup: wifi not connected - %s", wifiConfig.wl_status_to_name(status));
  logInput(buf);

  return false;
}

void set_static_ip_config()
{
  bool configOK = true;
  IPAddress staticIP;
  IPAddress gateway;
  IPAddress subnet;
  IPAddress dns1;
  IPAddress dns2;

  if (!staticIP.fromString(wifiConfig.ip))
  {
    configOK = false;
  }
  if (!gateway.fromString(wifiConfig.gateway))
  {
    configOK = false;
  }
  if (!subnet.fromString(wifiConfig.subnet))
  {
    configOK = false;
  }
  if (!dns1.fromString(wifiConfig.dns1))
  {
    configOK = false;
  }
  if (!dns2.fromString(wifiConfig.dns2))
  {
    configOK = false;
  }
  if (configOK)
  {
    if (!WiFi.config(staticIP, gateway, subnet, dns1, dns2))
    {
      logInput("Static IP config NOK");
    }
    else
    {
      logInput("Static IP config OK");
    }
  }
}
void initSensor()
{

  if (systemConfig.syssht30 == 1)
  {
    if (xSemaphoreTake(mutexI2Ctask, (TickType_t)500 / portTICK_PERIOD_MS) == pdTRUE)
    {
    }
    else
    {
      logInput("Error: SHT i2c semaphore not available");
      return;
    }

    if (sht_org.init() && sht_org.readSample())
    {
      SHT3x_original = true;
    }
    else if (sht_alt.init() && sht_alt.readSample())
    {
      SHT3x_alternative = true;
    }
    if (SHT3x_original)
    {
      logInput("Setup: Original SHT30 sensor found");
    }
    else if (SHT3x_alternative)
    {
      logInput("Setup: Alternative SHT30 sensor found");
    }
    if (SHT3x_original || SHT3x_alternative)
    {
      xSemaphoreGive(mutexI2Ctask);
      return;
    }

    delay(200);

    if (sht_org.init() && sht_org.readSample())
    {
      SHT3x_original = true;
    }
    else if (sht_alt.init() && sht_alt.readSample())
    {
      SHT3x_alternative = true;
    }
    if (SHT3x_original)
    {
      logInput("Setup: Original SHT30 sensor found (2nd try)");
    }
    else if (SHT3x_alternative)
    {
      logInput("Setup: Alternative SHT30 sensor found (2nd try)");
    }
    else
    {
      systemConfig.syssht30 = 0;
      logInput("Setup: SHT30 sensor not present");
    }
    xSemaphoreGive(mutexI2Ctask);
  }
}

void init_vRemote()
{
  // setup virtual remote
  char buff[128]{};
  sprintf(buff, "Setup: Virtual remotes, start ID: %02X,%02X,%02X - No.: %d", sys.getMac(3), sys.getMac(4), sys.getMac(5), systemConfig.itho_numvrem);
  logInput(buff);

  virtualRemotes.setMaxRemotes(systemConfig.itho_numvrem);
  loadVirtualRemotesConfig();
}

bool ithoInitCheck()
{
#if defined(CVE)
  if (digitalRead(STATUSPIN) == LOW)
  {
    return false;
  }
  if (systemConfig.itho_pwminit_en)
  {
    sendI2CPWMinit();
  }
#endif
  return false;
}

void update_queue()
{
  ithoQueue.update_queue();
}

// Update itho Value
bool writeIthoVal(uint16_t value, volatile uint16_t *ithoCurrentVal, bool *updateIthoMQTT)
{
  if (value > 254)
    return false;

  if (*ithoCurrentVal != value)
  {

    if (xSemaphoreTake(mutexI2Ctask, (TickType_t)500 / portTICK_PERIOD_MS) == pdTRUE)
    {
    }
    else
    {
      return false;
    }

    IthoPWMcommand(value, ithoCurrentVal, updateIthoMQTT);

    xSemaphoreGive(mutexI2Ctask);

    return true;
  }
  return false;
}

bool ithoI2CCommand(uint8_t remoteIndex, const char *command, cmdOrigin origin)
{
  D_LOG("EXEC VREMOTE BUTTON COMMAND:%s remote:%d\n", command, remoteIndex);

  if (xSemaphoreTake(mutexI2Ctask, (TickType_t)500 / portTICK_PERIOD_MS) == pdTRUE)
  {
  }
  else
  {
    return false;
  }

  bool updateweb = false;
  if (origin == WEB)
    updateweb = true;

  if (strcmp(command, "away") == 0)
  {
    sendRemoteCmd(remoteIndex, IthoAway, virtualRemotes);
  }
  else if (strcmp(command, "low") == 0)
  {
    sendRemoteCmd(remoteIndex, IthoLow, virtualRemotes);
  }
  else if (strcmp(command, "medium") == 0)
  {
    sendRemoteCmd(remoteIndex, IthoMedium, virtualRemotes);
  }
  else if (strcmp(command, "high") == 0)
  {
    sendRemoteCmd(remoteIndex, IthoHigh, virtualRemotes);
  }
  else if (strcmp(command, "timer1") == 0)
  {
    sendRemoteCmd(remoteIndex, IthoTimer1, virtualRemotes);
  }
  else if (strcmp(command, "timer2") == 0)
  {
    sendRemoteCmd(remoteIndex, IthoTimer2, virtualRemotes);
  }
  else if (strcmp(command, "timer3") == 0)
  {
    sendRemoteCmd(remoteIndex, IthoTimer3, virtualRemotes);
  }
  else if (strcmp(command, "cook30") == 0)
  {
    sendRemoteCmd(remoteIndex, IthoCook30, virtualRemotes);
  }
  else if (strcmp(command, "cook60") == 0)
  {
    sendRemoteCmd(remoteIndex, IthoCook60, virtualRemotes);
  }
  else if (strcmp(command, "auto") == 0)
  {
    sendRemoteCmd(remoteIndex, IthoAuto, virtualRemotes);
  }
  else if (strcmp(command, "autonight") == 0)
  {
    sendRemoteCmd(remoteIndex, IthoAutoNight, virtualRemotes);
  }
  else if (strcmp(command, "join") == 0)
  {
    sendRemoteCmd(remoteIndex, IthoJoin, virtualRemotes);
  }
  else if (strcmp(command, "leave") == 0)
  {
    sendRemoteCmd(remoteIndex, IthoLeave, virtualRemotes);
  }
  else if (strcmp(command, "type") == 0)
  {
    sendQueryDevicetype(updateweb);
  }
  else if (strcmp(command, "status") == 0)
  {
    sendQueryStatus(updateweb);
  }
  else if (strcmp(command, "statusformat") == 0)
  {
    sendQueryStatusFormat(updateweb);
  }
  else if (strcmp(command, "31DA") == 0)
  {
    sendQuery31DA(updateweb);
  }
  else if (strcmp(command, "31D9") == 0)
  {
    sendQuery31D9(updateweb);
  }
  else if (strcmp(command, "10D0") == 0)
  {
    filterReset(0, virtualRemotes);
  }
  else if (strcmp(command, "shtreset") == 0)
  {
    reset_sht_sensor = true;
  }
  else
  {

    xSemaphoreGive(mutexI2Ctask);

    return false;
  }

  const char *source;
  auto it = cmdOriginMap.find(origin);
  if (it != cmdOriginMap.end())
    source = it->second;
  else
    source = cmdOriginMap.rbegin()->second;

  char originchar[30]{};
  sprintf(originchar, "%s-vremote-%d", source, remoteIndex);

  logLastCommand(command, originchar);

  xSemaphoreGive(mutexI2Ctask);

  return true;
}
