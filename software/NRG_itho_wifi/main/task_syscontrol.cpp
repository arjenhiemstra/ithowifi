

#include "task_syscontrol.h"

#define TASK_SYS_CONTROL_PRIO 6

// globals
uint32_t TaskSysControlHWmark = 0;
DNSServer dnsServer;
bool SHT3x_original = false;
bool SHT3x_alternative = false;
bool runscan = false;
bool dontSaveConfig = false;
bool saveSystemConfigflag = false;
bool saveLogConfigflag = false;
bool saveWifiConfigflag = false;
bool resetWifiConfigflag = false;
bool resetSystemConfigflag = false;
bool clearQueue = false;
bool shouldReboot = false;
int8_t ithoInitResult = 0;
bool IthoInit = false;
bool wifiModeAP = false;
bool reset_sht_sensor = false;
bool restest = false;
bool i2c_safe_guard_log = true;

// locals
StaticTask_t xTaskSysControlBuffer;
StackType_t xTaskSysControlStack[STACK_SIZE_MEDIUM];
TaskHandle_t xTaskSysControlHandle = NULL;

unsigned long lastI2CinitRequest = 0;
bool ithoInitResultLogEntry = true;
Ticker scan;

unsigned long lastWIFIReconnectAttempt = 0;
unsigned long APmodeTimeout = 0;
unsigned long wifiLedUpdate = 0;
unsigned long SHT3x_readout = 0;
unsigned long query2401tim = 0;
unsigned long mqttUpdatetim = 0;
unsigned long lastVersionCheck = 0;
bool i2c_init_functions_done = false;
bool joinSend = false;

SHTSensor sht_org(SHTSensor::SHT3X);
SHTSensor sht_alt(SHTSensor::SHT3X_ALT);

void startTaskSysControl()
{
  xTaskSysControlHandle = xTaskCreateStaticPinnedToCore(
      TaskSysControl,
      "TaskSysControl",
      STACK_SIZE_MEDIUM,
      (void *)1,
      TASK_SYS_CONTROL_PRIO,
      xTaskSysControlStack,
      &xTaskSysControlBuffer,
      CONFIG_ARDUINO_RUNNING_CORE);
}

void work_i2c_queue()
{

  if (!i2c_cmd_queue.empty())
  {
    if (i2c_safe_guard.i2c_safe_guard_enabled == true && i2c_safe_guard.sniffer_enabled == false)
    {

      // block 500ms before expected temp readout and 500ms after expected temp readout
      unsigned long cur_time = millis();
      if (i2c_safe_guard.start_close_time < cur_time && cur_time < i2c_safe_guard.end_close_time)
      {
        if (i2c_safe_guard_log)
        {
          i2c_safe_guard_log = false;
          // D_LOG("i2c_safe_guard blocked queue: start:%d > cur:%d < end:%d", i2c_safe_guard.start_close_time, cur_time, i2c_safe_guard.end_close_time);
        }
        return;
      }
    }
    if (!i2c_safe_guard_log)
    {
      // D_LOG("i2c_safe_guard unblocked queue");
      i2c_safe_guard_log = true;
    }
    i2c_cmd_queue.front()(); //()() is there for a reason!
    i2c_cmd_queue.pop_front();
  }
}

void TaskSysControl(void *pvParameters)
{
  configASSERT((uint32_t)pvParameters == 1UL);
  Ticker TaskTimeout;
  Ticker queueUpdater;

  wifiInit();

  syslog.appName(logConfig.logref);
  syslog.deviceHostname(hostName());
  syslog.server(logConfig.logserver, logConfig.logport);
  syslog.defaultPriority(LOG_KERN);

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
                        { W_LOG("Warning: Task SysControl timed out!"); });

    execSystemControlTasks();

    if (shouldReboot)
    {
      TaskTimeout.detach();
      N_LOG("Reboot requested");
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
  work_i2c_queue();

  if (IthoInit && millis() > 250)
  {
    IthoInit = ithoInitCheck();
  }

  if (!i2c_init_functions_done)
  {
    init_i2c_functions();
  }
  // request itho status every systemConfig.itho_updatefreq sec.
  else if (millis() - query2401tim >= systemConfig.itho_updatefreq * 1000UL)
  {
    query2401tim = millis();
    bool _updateMQTT = false;
    if (systemConfig.itho_2401 == 1)
    {
      _updateMQTT = true;
      i2c_queue_add_cmd([]()
                        { sendQueryStatus(false); });
    }
    if (systemConfig.itho_31da == 1)
    {
      _updateMQTT = true;
      i2c_queue_add_cmd([]()
                        { sendQuery31DA(false); });
    }
    if (systemConfig.itho_31d9 == 1)
    {
      _updateMQTT = true;
      i2c_queue_add_cmd([]()
                        { sendQuery31D9(false); });
    }
    if (_updateMQTT)
    {
      i2c_queue_add_cmd([]()
                        { updateMQTTihtoStatus = true; });
    }
  }

  if (systemConfig.itho_sendjoin > 0 && !joinSend && ithoInitResult == 1)
  {
    joinSend = true;
    i2c_queue_add_cmd([]()
                      { sendRemoteCmd(0, IthoJoin); });

    I_LOG("I2C init: Virtual remote join command send");

    if (systemConfig.itho_sendjoin == 1)
    {
      systemConfig.itho_sendjoin = 0;
      saveSystemConfig();
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
    snprintf(buf, sizeof(buf), "speed:%d", speed);

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
  }
  // System control tasks

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
  else if (WiFi.status() != WL_CONNECTED)
  {
    if (millis() - lastWIFIReconnectAttempt > 60000)
    {
      I_LOG("Attempt to reconnect WiFi");
      lastWIFIReconnectAttempt = millis();
      // Attempt to reconnect
      if (connectWiFiSTA())
      {
        I_LOG("Reconnect WiFi successful");
        lastWIFIReconnectAttempt = 0;
      }
      else
      {
        E_LOG("Reconnect WiFi failed!");
      }
    }
  }
  if (runscan)
  {
    runscan = false;
    scan.once_ms(10, wifiScan);
  }

  if (!(currentItho_fwversion() > 0) && millis() - lastVersionCheck > 60000)
  {
    lastVersionCheck = millis();
    i2c_queue_add_cmd([]()
                      { sendQueryDevicetype(false); });
  }

  if (ithoInitResult == -1)
  {
    if (millis() - mqttUpdatetim >= systemConfig.itho_updatefreq * 1000UL)
    {
      mqttUpdatetim = millis();
      updateMQTTihtoStatus = true;
    }
  }
  if (systemConfig.syssht30 == 1)
  {
    if (millis() - SHT3x_readout >= systemConfig.itho_updatefreq * 1000UL && (SHT3x_original || SHT3x_alternative))
    {
      SHT3x_readout = millis();

      if (SHT3x_original || SHT3x_alternative)
      {
        i2c_queue_add_cmd([]()
                          { update_sht_sensor(); });
      }
    }
  }
  if (reset_sht_sensor && (!SHT3x_alternative && !SHT3x_original))
  {
    reset_sht_sensor = false;
    trigger_sht_sensor_reset();
    ithoInitResult = 0;
    ithoInitResultLogEntry = true;
    i2c_init_functions_done = false;
    sysStatReq = true;
    N_LOG("SHT3x sensor reset: executed");
    jsonSysmessage("i2c_sht_reset", "Done");
  }
}

void update_sht_sensor()
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
        ithoInitResultLogEntry = true;
        i2c_init_functions_done = false;
        sysStatReq = true;
      }
      N_LOG("SHT3x sensor reset: %s", reset_res ? "Success" : "Failed");
      char buf[32]{};
      snprintf(buf, sizeof(buf), "%s", reset_res ? "OK" : "NOK");
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
        ithoInitResultLogEntry = true;
        i2c_init_functions_done = false;
        sysStatReq = true;
      }
      N_LOG("SHT3x sensor reset: %s", reset_res ? "Success" : "Failed");
      char buf[32]{};
      snprintf(buf, sizeof(buf), "%s", reset_res ? "OK" : "NOK");
      jsonSysmessage("i2c_sht_reset", buf);
    }
    else if (sht_alt.readSample())
    {
      ithoHum = sht_alt.getHumidity();
      ithoTemp = sht_alt.getTemperature();
    }
  }
}

void init_i2c_functions()
{
  if (millis() > 15000 && (millis() - lastI2CinitRequest > 5000))
  {
    lastI2CinitRequest = millis();
    sendQueryDevicetype(false);

    if (currentItho_fwversion() > 0)
    {
      N_LOG("I2C init: QueryDevicetype - fw:%d hw:%d", currentItho_fwversion(), currentIthoDeviceID());

      ithoInitResult = 1;
      i2c_init_functions_done = true;

      if (hardware_rev_det == 0x3F || hardware_rev_det == 0x03) // CVE
      {
        digitalWrite(itho_status_pin, HIGH);
      }
      else // NON-CVE
      {
        digitalWrite(status_pin, HIGH);
      }
      if (systemConfig.syssht30 > 0)
      {
        if (currentIthoDeviceID() == 0x1B)
        {
          uint8_t index = 0;
          int32_t value = 0;

          if (systemConfig.syssht30 == 1)
          {
            /*
               switch itho hum setting off on every boot because
               hum sensor setting gets restored in itho firmware after power cycle
            */
            value = 0;
          }

          else if (systemConfig.syssht30 == 2)
          {
            /*
               if value == 2 setting changed from 1 -> 0
               then restore itho hum setting back to "on"
            */
            value = 1;
          }
          if (currentItho_fwversion() == 25)
          {
            index = 63;
          }
          else if (currentItho_fwversion() == 26 || currentItho_fwversion() == 27)
          {
            index = 71;
          }
          if (index > 0)
          {
            i2c_queue_add_cmd([index]()
                              { sendQuery2410(index, false); });
            i2c_queue_add_cmd([index, value]()
                              { setSetting2410(index, value, false); });

            N_LOG("I2C init: set hum sensor in itho firmware to: %s", value ? "on" : "off");
          }
        }
        if (systemConfig.syssht30 == 2)
        {
          systemConfig.syssht30 = 0;
          saveSystemConfig();
        }
      }

      sendQueryStatusFormat(false);
      N_LOG("I2C init: QueryStatusFormat - items:%lu", static_cast<unsigned long>(ithoStatus.size()));

      if (systemConfig.itho_2401 == 1)
      {
        sendQueryStatus(false);
        N_LOG("I2C init: QueryStatus");
      }

      if (i2c_sniffer_capable)
      {
        if (systemConfig.i2c_safe_guard == 1)
        {
          N_LOG("I2C init: Safe guard enabled");
          i2c_safe_guard.i2c_safe_guard_enabled = true;
          i2c_sniffer_enable();
        }
        if (systemConfig.i2c_sniffer == 1)
        {
          N_LOG("I2C init: i2c sniffer enabled");
          i2c_safe_guard.sniffer_enabled = true;
          i2c_sniffer_enable();
        }
      }

      sendHomeAssistantDiscovery = true;
    }
    else
    {
      if (ithoInitResultLogEntry)
      {
        if (ithoInitResult != -2)
        {
          ithoInitResult = -1;
        }
        ithoInitResultLogEntry = false;
        E_LOG("I2C init: QueryDevicetype - failed");
      }
    }
  }
}

void wifiInit()
{
  if (!loadWifiConfig())
  {
    E_LOG("Setup: Wifi config load failed");
    setupWiFiAP();
  }
  else if (!connectWiFiSTA(true))
  {
    E_LOG("Setup: Wifi connect STA failed");
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
    N_LOG("Setup: AP mode active");
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
  N_LOG("wifi AP mode started");
}

bool connectWiFiSTA(bool restore)
{

  wifiModeAP = false;
  D_LOG("Connecting to wireless network...");

  // Do not use SDK storage of SSID/WPA parameters
  WiFi.persistent(false);

#ifdef ESPRESSIF32_3_5_0_WIFI
  //
  // Do not use flash storage for wifi settings
  esp_err_t wifi_set_storage = esp_wifi_set_storage(WIFI_STORAGE_RAM);
  D_LOG("esp_wifi_set_storage: %s", esp_err_to_name(wifi_set_storage));
  // Disconnect any existing connections and clear
  if (!WiFi.disconnect(true))
    E_LOG("Unable to set wifi disconnect");

  // WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE);
  //  Set hostname
  bool setHostname_result = WiFi.setHostname(hostName());
  D_LOG("WiFi.setHostname: %s", setHostname_result ? "OK" : "NOK");

  // No AutoReconnect
  if (!WiFi.setAutoReconnect(false))
    E_LOG("Unable to set auto reconnect");
  delay(200);
  // Set correct mode
  if (!WiFi.mode(WIFI_STA))
    E_LOG("Unable to set WiFi mode");

  // No power saving
  esp_err_t wifi_set_ps = esp_wifi_set_ps(WIFI_PS_NONE);
  D_LOG("esp_wifi_set_ps: %s", esp_err_to_name(wifi_set_ps));
  //
#else
  // Set wifi mode to STA for next step (clear config)
  if (!WiFi.mode(WIFI_STA))
    E_LOG("Unable to set WiFi mode to STA");

  // Clear any saved wifi config
  if (restore)
  {
    esp_err_t wifi_restore = esp_wifi_restore();
    D_LOG("esp_wifi_restore: %s", esp_err_to_name(wifi_restore));
  }
  // Disconnect any existing connections and clear
  if (!WiFi.disconnect(true, true))
    E_LOG("Unable to set wifi disconnect");

  // Reset wifi mode to NULL
  if (!WiFi.mode(WIFI_MODE_NULL))
    E_LOG("Unable to set WiFi mode to NULL");

  // Set hostname
  bool setHostname_result = WiFi.setHostname(hostName());
  D_LOG("WiFi.setHostname: %s", setHostname_result ? "OK" : "NOK");

  // No AutoReconnect
  if (!WiFi.setAutoReconnect(false))
    E_LOG("Unable to set auto reconnect");

  delay(200);

  // Begin init of actual wifi connection

  // Set correct mode
  if (!WiFi.mode(WIFI_STA))
    E_LOG("Unable to set WiFi mode");

  esp_err_t esp_wifi_set_max_tx_power(int8_t power);
  esp_err_t wifi_set_max_tx_power = esp_wifi_set_max_tx_power(78);
  D_LOG("esp_wifi_set_max_tx_power: %s", esp_err_to_name(wifi_set_max_tx_power));

  int8_t wifi_power_level = -1;
  esp_err_t wifi_get_max_tx_power = esp_wifi_get_max_tx_power(&wifi_power_level);
  D_LOG("esp_wifi_get_max_tx_power: %s - level:%d", esp_err_to_name(wifi_get_max_tx_power), wifi_power_level);

  // Do not use flash storage for wifi settings
  esp_err_t wifi_set_storage = esp_wifi_set_storage(WIFI_STORAGE_RAM);
  D_LOG("esp_wifi_set_storage: %s", esp_err_to_name(wifi_set_storage));

  // No power saving
  esp_err_t wifi_set_ps = esp_wifi_set_ps(WIFI_PS_NONE);
  D_LOG("esp_wifi_set_ps: %s", esp_err_to_name(wifi_set_ps));

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

  WiFi.begin(wifiConfig.ssid, wifiConfig.passwd);

  auto timeoutmillis = millis() + 30000;
  wl_status_t status = WiFi.status();

  while (millis() < timeoutmillis)
  {

    esp_task_wdt_reset();

    status = WiFi.status();
    if (status == WL_CONNECTED)
    {
      digitalWrite(wifi_led_pin, LOW);
      return true;
    }
    if (digitalRead(wifi_led_pin) == LOW)
    {
      digitalWrite(wifi_led_pin, HIGH);
    }
    else
    {
      digitalWrite(wifi_led_pin, LOW);
    }

    delay(100);
  }

  digitalWrite(wifi_led_pin, HIGH);

  E_LOG("Setup: wifi not connected - %s", wifiConfig.wl_status_to_name(status));

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
      E_LOG("Static IP config NOK");
    }
    else
    {
      I_LOG("Static IP config OK");
    }
  }
}
void initSensor()
{

  if (systemConfig.syssht30 == 1)
  {

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
      N_LOG("Setup: Original SHT30 sensor found");
    }
    else if (SHT3x_alternative)
    {
      N_LOG("Setup: Alternative SHT30 sensor found");
    }
    if (SHT3x_original || SHT3x_alternative)
    {
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
      W_LOG("Setup: Original SHT30 sensor found (2nd try)");
    }
    else if (SHT3x_alternative)
    {
      W_LOG("Setup: Alternative SHT30 sensor found (2nd try)");
    }
    else
    {
      systemConfig.syssht30 = 0;
      N_LOG("Setup: SHT30 sensor not present");
    }
  }
}

void init_vRemote()
{
  // setup virtual remote
  I_LOG("Setup: Virtual remotes, start ID: %02X,%02X,%02X - No.: %d", sys.getMac(3), sys.getMac(4), sys.getMac(5), systemConfig.itho_numvrem);

  virtualRemotes.setMaxRemotes(systemConfig.itho_numvrem);
  loadVirtualRemotesConfig();
}

bool ithoInitCheck()
{
  if (hardware_rev_det == 0x3F || hardware_rev_det == 0x03) // CVE
  {
    if (digitalRead(status_pin) == LOW)
    {
      return false;
    }
    if (systemConfig.itho_pwm2i2c)
    {
      sendI2CPWMinit();
    }
  }
  return false;
}

void update_queue()
{
  ithoQueue.update_queue();
}
// Update itho Value
bool writeIthoVal(uint16_t value, volatile uint16_t *ithoCurrentVal, bool *updateIthoMQTT)
{
  if (systemConfig.itho_pwm2i2c != 1)
    return false;
  if (value > 256)
    return false;

  if (*ithoCurrentVal != value)
  {

    i2c_queue_add_cmd([value, ithoCurrentVal, updateIthoMQTT]()
                      { IthoPWMcommand(value, ithoCurrentVal, updateIthoMQTT); });

    return true;
  }
  return false;
}

void ithoI2CCommand(uint8_t remoteIndex, const char *command, cmdOrigin origin)
{
  D_LOG("EXEC VREMOTE BUTTON COMMAND:%s remote:%d", command, remoteIndex);

  bool updateweb = false;
  if (origin == WEB)
    updateweb = true;

  if (strcmp(command, "away") == 0)
  {
    i2c_queue_add_cmd([remoteIndex]()
                      { sendRemoteCmd(remoteIndex, IthoAway); });
  }
  else if (strcmp(command, "low") == 0)
  {
    i2c_queue_add_cmd([remoteIndex]()
                                   { sendRemoteCmd(remoteIndex, IthoLow); });
  }
  else if (strcmp(command, "medium") == 0)
  {
    i2c_queue_add_cmd([remoteIndex]()
                      { sendRemoteCmd(remoteIndex, IthoMedium); });
  }
  else if (strcmp(command, "high") == 0)
  {
    i2c_queue_add_cmd([remoteIndex]()
                      { sendRemoteCmd(remoteIndex, IthoHigh); });
  }
  else if (strcmp(command, "timer1") == 0)
  {
    i2c_queue_add_cmd([remoteIndex]()
                      { sendRemoteCmd(remoteIndex, IthoTimer1); });
  }
  else if (strcmp(command, "timer2") == 0)
  {
    i2c_queue_add_cmd([remoteIndex]()
                      { sendRemoteCmd(remoteIndex, IthoTimer2); });
  }
  else if (strcmp(command, "timer3") == 0)
  {
    i2c_queue_add_cmd([remoteIndex]()
                      { sendRemoteCmd(remoteIndex, IthoTimer3); });
  }
  else if (strcmp(command, "cook30") == 0)
  {
    i2c_queue_add_cmd([remoteIndex]()
                      { sendRemoteCmd(remoteIndex, IthoCook30); });
  }
  else if (strcmp(command, "cook60") == 0)
  {
    i2c_queue_add_cmd([remoteIndex]()
                      { sendRemoteCmd(remoteIndex, IthoCook60); });
  }
  else if (strcmp(command, "auto") == 0)
  {
    i2c_queue_add_cmd([remoteIndex]()
                      { sendRemoteCmd(remoteIndex, IthoAuto); });
  }
  else if (strcmp(command, "autonight") == 0)
  {
    i2c_queue_add_cmd([remoteIndex]()
                      { sendRemoteCmd(remoteIndex, IthoAutoNight); });
  }
  else if (strcmp(command, "join") == 0)
  {
    i2c_queue_add_cmd([remoteIndex]()
                            { sendRemoteCmd(remoteIndex, IthoJoin); });
  }
  else if (strcmp(command, "leave") == 0)
  {
    i2c_queue_add_cmd([remoteIndex]()
                      { sendRemoteCmd(remoteIndex, IthoLeave); });
  }
  else if (strcmp(command, "type") == 0)
  {
    i2c_queue_add_cmd([updateweb]()
                            { sendQueryDevicetype(updateweb); });
  }
  else if (strcmp(command, "status") == 0)
  {
    i2c_queue_add_cmd([updateweb]()
                      { sendQueryStatus(updateweb); });
  }
  else if (strcmp(command, "statusformat") == 0)
  {
    i2c_queue_add_cmd([updateweb]()
                            { sendQueryStatusFormat(updateweb); });
  }
  else if (strcmp(command, "31DA") == 0)
  {
    i2c_queue_add_cmd([updateweb]()
                            { sendQuery31DA(updateweb); });
  }
  else if (strcmp(command, "31D9") == 0)
  {
    i2c_queue_add_cmd([updateweb]()
                            { sendQuery31D9(updateweb); });
  }
  else if (strcmp(command, "10D0") == 0)
  {
    i2c_queue_add_cmd([]()
                      { filterReset(); });
  }
  else if (strcmp(command, "shtreset") == 0)
  {
    reset_sht_sensor = true;
  }

  const char *source;
  auto it = cmdOriginMap.find(origin);
  if (it != cmdOriginMap.end())
    source = it->second;
  else
    source = cmdOriginMap.rbegin()->second;

  char originchar[30]{};
  snprintf(originchar, sizeof(originchar), "%s-vremote-%d", source, remoteIndex);

  logLastCommand(command, originchar);
}
