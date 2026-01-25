

#include "tasks/task_syscontrol.h"
#include "../sys_log.h"

#define TASK_SYS_CONTROL_PRIO 6
// globals
uint32_t TaskSysControlHWmark = 0;
DNSServer dnsServer;
bool SHT3x_original = false;
bool SHT3x_alternative = false;
bool runscan = false;
bool saveSystemConfigflag = false;
bool saveLogConfigflag = false;
bool saveWifiConfigflag = false;
bool saveHADiscConfigflag = false;
bool resetWifiConfigflag = false;
bool resetSystemConfigflag = false;
bool resetHADiscConfigflag = false;
bool clearQueue = false;
bool shouldReboot = false;
int8_t ithoInitResult = 0;
bool ithoStatusFormateSuccessful = false;
bool IthoInit = false;
bool wifiModeAP = false;
bool wifiSTAshouldConnect = false;
bool wifiSTAconnected = false;
bool wifiSTAconnecting = false;
bool reset_sht_sensor = false;
bool restest = false;
bool i2c_safe_guard_log = true;
unsigned long getFWupdateInfo = 0;
struct firmwareinfo firmwareInfo;

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
unsigned long query4210tim = 0;
unsigned long mqttUpdatetim = 0;
unsigned long lastVersionCheck = 0;
unsigned long lastIthoStatusFormatCheck = 0;
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

  client.setInsecure(); // skip CA verification

  delay(2000);
  wifiInit();
  delay(2000);
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
                        { W_LOG("SYS: warning - Task SysControl timed out!"); });

    execSystemControlTasks();

    if (shouldReboot)
    {
      TaskTimeout.detach();
      N_LOG("SYS: reboot requested");
      delay(1000);
      ACTIVE_FS.end();
      esp_restart();
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
  // request itho counter status every systemConfig.itho_counter_updatefreq sec.
  else if (millis() - query4210tim >= systemConfig.itho_counter_updatefreq * 1000UL)
  {
    query4210tim = millis();
    if (systemConfig.itho_4210 == 1)
    {
      i2c_queue_add_cmd([]()
                        { sendQueryCounters(false); });
      i2c_queue_add_cmd([]()
                        { updateMQTTihtoStatus = true; });
    }
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
      saveSystemConfig("flash");
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
      if (strcmp(lastCmd.source, "") == 0)
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
    if (wifiConfig.aptimeout > 0 && wifiConfig.aptimeout < 255)
    {
      if (millis() - APmodeTimeout > (wifiConfig.aptimeout * 60 * 1000))
      {
        if (!wifiSTAconnected) // reset timout
          APmodeTimeout = millis();
        else // disable AP after wifiConfig.aptimeout min
          WiFi.enableAP(false);
      }
    }
    dnsServer.processNextRequest();
    if (!wifiSTAconnected && !wifiSTAconnecting)
    {
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
  }
  if (WiFi.status() != WL_CONNECTED && wifiSTAshouldConnect && !wifiSTAconnecting)
  {
    if (digitalRead(wifi_led_pin) == LOW)
    {
      digitalWrite(wifi_led_pin, HIGH);
    }
    if (millis() - lastWIFIReconnectAttempt > 60000)
    {
      I_LOG("NET: attempt to reconnect WiFi");
      lastWIFIReconnectAttempt = millis();
      // Attempt to reconnect
      if (connectWiFiSTA())
      {
        I_LOG("NET: WiFi reconnect successful");
        lastWIFIReconnectAttempt = 0;
      }
      else
      {
        E_LOG("NET: error - WiFi reconnect failed!");
      }
    }
  }
  if (WiFi.status() == WL_CONNECTED && wifiSTAshouldConnect)
  {
    if (digitalRead(wifi_led_pin) == HIGH)
    {
      digitalWrite(wifi_led_pin, LOW);
    }
  }
  if (runscan)
  {
    runscan = false;
    scan.once_ms(10, wifiScan);
  }

  // If the status format is not retrieved correctly during boot but lost during runtime; resend the query.
  if ((ithoStatus.empty() && systemConfig.itho_2401 == 1) && millis() - lastIthoStatusFormatCheck > 60000 && ithoStatusFormateSuccessful)
  {
    lastIthoStatusFormatCheck = millis();
    sendQueryStatusFormat(false);
    E_LOG("I2C: error - StatusFormat lost. Resend query result: %s", ithoStatus.size()?"success":"failed");
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
    N_LOG("SYS: SHT3x sensor reset: executed");
    jsonSysmessage("i2c_sht_reset", "Done");
  }
  if (WiFi.status() == WL_CONNECTED && systemConfig.fw_check && (millis() - getFWupdateInfo > 24 * 60 * 60 * 1000)) // 24hr
  {
    getFWupdateInfo = millis();
    check_firmware_update();
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
      N_LOG("SYS: SHT3x sensor reset: %s", reset_res ? "Success" : "Failed");
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
      N_LOG("SYS: SHT3x sensor reset: %s", reset_res ? "Success" : "Failed");
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
      N_LOG("I2C: QueryDevicetype - mfr:0x%02X type:0x%02X fw:0x%02X hw:0x%02X", currentIthoDeviceGroup(), currentIthoDeviceID(), currentItho_fwversion(), currentItho_hwversion());

      ithoInitResult = 1;
      i2c_init_functions_done = true;

      digitalWrite(status_pin, HIGH);
      if (systemConfig.rfInitOK)
      {
        delay(250);
        digitalWrite(status_pin, LOW);
        delay(250);
        digitalWrite(status_pin, HIGH);
        delay(250);
        digitalWrite(status_pin, LOW);
        delay(250);
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

            N_LOG("I2C: set hum sensor in itho firmware to: %s", value ? "on" : "off");
          }
        }
        if (systemConfig.syssht30 == 2)
        {
          systemConfig.syssht30 = 0;
          N_LOG("I2C: hum sensor setting set to \"off\"");
          saveSystemConfig("flash");
        }
      }

      sendQueryStatusFormat(false);
      N_LOG("I2C: QueryStatusFormat - items:%lu", static_cast<unsigned long>(ithoStatus.size()));
      if(ithoStatus.size() > 0) ithoStatusFormateSuccessful = true;

      if (systemConfig.itho_2401 == 1)
      {
        sendQueryStatus(false);
        N_LOG("I2C: QueryStatus");
      }

      if (systemConfig.i2c_safe_guard > 0 && currentIthoDeviceID() == 0x1B)
      {
        if (i2c_sniffer_capable)
        {
          if (systemConfig.syssht30 == 0 && currentItho_fwversion() >= 25)
          {
            N_LOG("I2C: safe guard enabled");
            i2c_safe_guard.i2c_safe_guard_enabled = true;
            i2c_sniffer_enable();
          }
          else
          {
            if (systemConfig.i2c_safe_guard == 1)
            {
              W_LOG("I2C: i2c safe guard could not be enabled");
            }
          }
        }
        else
        {
          if (systemConfig.i2c_safe_guard == 1)
          {
            E_LOG("I2C: i2c safe guard needs sniffer capable hardware");
          }
        }
      }
      else
      {
        if (systemConfig.i2c_safe_guard == 1)
        {
          E_LOG("I2C init: i2c safe guard needs supported itho device to work");
        }
      }
      if (systemConfig.i2c_sniffer == 1)
      {
        if (!i2c_sniffer_capable)
        {
          E_LOG("I2C init: i2c sniffer needs sniffer capable hardware");
        }
        else
        {
          N_LOG("I2C: i2c sniffer enabled");
          i2c_safe_guard.sniffer_enabled = true;
          i2c_sniffer_enable();
        }
      }
      if (HADiscConfigLoaded && (strcmp(haDiscConfig.d, "unset") == 0))
      {
        strncpy(haDiscConfig.d, getIthoType(), sizeof(haDiscConfig.d));
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
        E_LOG("I2C: QueryDevicetype - failed");
      }
    }
  }
}

void wifiInit()
{
  WifiConfigLoaded = loadWifiConfig("flash");
  if (!WifiConfigLoaded)
  {
    E_LOG("SYS: Wifi config load failed");
  }

  WiFi.onEvent(WiFiEvent);
  setupWiFigeneric();

  if (wifiConfig.aptimeout > 0)
  {
    I_LOG("SYS: starting wifi access point");
    setupWiFiAP();
  }
  if (strcmp(wifiConfig.ssid, "") != 0)
  {
    connectWiFiSTA();
  }
  else
  {
    I_LOG("SYS: initial wifi config still there");
  }

  if (strcmp(wifiConfig.ntpserver, "") != 0)
  {
    configTime(0, 0, wifiConfig.ntpserver);
  }

  // set timezone

  const char *tz_string = wifiConfig.getTimeZoneStr();

  N_LOG("SYS: timezone: %s, specifier %s ", wifiConfig.timezone, tz_string);
  setenv("TZ", tz_string, 1);
  tzset();

  WiFi.scanDelete();
  if (WiFi.scanComplete() == -2)
  {
    WiFi.scanNetworks(true);
  }
}

void WiFiEvent(WiFiEvent_t event, arduino_event_info_t info)
{
  /*
  * WiFi Events

  0  ARDUINO_EVENT_WIFI_READY               < ESP32 WiFi ready
  1  ARDUINO_EVENT_WIFI_SCAN_DONE                < ESP32 finish scanning AP
  2  ARDUINO_EVENT_WIFI_STA_START                < ESP32 station start
  3  ARDUINO_EVENT_WIFI_STA_STOP                 < ESP32 station stop
  4  ARDUINO_EVENT_WIFI_STA_CONNECTED            < ESP32 station connected to AP
  5  ARDUINO_EVENT_WIFI_STA_DISCONNECTED         < ESP32 station disconnected from AP
  6  ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE      < the auth mode of AP connected by ESP32 station changed
  7  ARDUINO_EVENT_WIFI_STA_GOT_IP               < ESP32 station got IP from connected AP
  8  ARDUINO_EVENT_WIFI_STA_LOST_IP              < ESP32 station lost IP and the IP is reset to 0
  9  ARDUINO_EVENT_WPS_ER_SUCCESS       < ESP32 station wps succeeds in enrollee mode
  10 ARDUINO_EVENT_WPS_ER_FAILED        < ESP32 station wps fails in enrollee mode
  11 ARDUINO_EVENT_WPS_ER_TIMEOUT       < ESP32 station wps timeout in enrollee mode
  12 ARDUINO_EVENT_WPS_ER_PIN           < ESP32 station wps pin code in enrollee mode
  13 ARDUINO_EVENT_WIFI_AP_START                 < ESP32 soft-AP start
  14 ARDUINO_EVENT_WIFI_AP_STOP                  < ESP32 soft-AP stop
  15 ARDUINO_EVENT_WIFI_AP_STACONNECTED          < a station connected to ESP32 soft-AP
  16 ARDUINO_EVENT_WIFI_AP_STADISCONNECTED       < a station disconnected from ESP32 soft-AP
  17 ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED         < ESP32 soft-AP assign an IP to a connected station
  18 ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED        < Receive probe request packet in soft-AP interface
  19 ARDUINO_EVENT_WIFI_AP_GOT_IP6               < ESP32 ap interface v6IP addr is preferred
  19 ARDUINO_EVENT_WIFI_STA_GOT_IP6              < ESP32 station interface v6IP addr is preferred
  20 ARDUINO_EVENT_ETH_START                < ESP32 ethernet start
  21 ARDUINO_EVENT_ETH_STOP                 < ESP32 ethernet stop
  22 ARDUINO_EVENT_ETH_CONNECTED            < ESP32 ethernet phy link up
  23 ARDUINO_EVENT_ETH_DISCONNECTED         < ESP32 ethernet phy link down
  24 ARDUINO_EVENT_ETH_GOT_IP               < ESP32 ethernet got IP from connected AP
  19 ARDUINO_EVENT_ETH_GOT_IP6              < ESP32 ethernet interface v6IP addr is preferred
  25 ARDUINO_EVENT_MAX
  */
  char buf[70]{};

  switch (event)
  {
  case ARDUINO_EVENT_WIFI_READY:
    strlcpy(buf, "WiFi interface ready", sizeof(buf));
    break;
  case ARDUINO_EVENT_WIFI_SCAN_DONE:
    strlcpy(buf, "Completed scan for access points", sizeof(buf));
    break;
  case ARDUINO_EVENT_WIFI_STA_START:
    strlcpy(buf, "WiFi client started", sizeof(buf));
    break;
  case ARDUINO_EVENT_WIFI_STA_STOP:
    wifiSTAshouldConnect = false;
    strlcpy(buf, "WiFi clients stopped", sizeof(buf));
    break;
  case ARDUINO_EVENT_WIFI_STA_CONNECTED:
    strlcpy(buf, "Connected to access point", sizeof(buf));
    break;
  case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
    wifiSTAconnected = false;
    wifiSTAshouldConnect = true;
    strlcpy(buf, "Disconnected from WiFi access point", sizeof(buf));
    break;
  case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
    strlcpy(buf, "Authentication mode of access point has changed", sizeof(buf));
    break;
  case ARDUINO_EVENT_WIFI_STA_GOT_IP:
  {
    wifiSTAconnected = true;
    logWifiInfo();
    strlcpy(buf, "WiFi client got IP", sizeof(buf));
    break;
  }
  case ARDUINO_EVENT_WIFI_STA_LOST_IP:
    wifiSTAconnected = false;
    wifiSTAshouldConnect = true;
    strlcpy(buf, "Lost IP address and IP address is reset to 0", sizeof(buf));
    break;
  case ARDUINO_EVENT_WPS_ER_SUCCESS:
    strlcpy(buf, "WiFi Protected Setup (WPS): succeeded in enrollee mode", sizeof(buf));
    break;
  case ARDUINO_EVENT_WPS_ER_FAILED:
    strlcpy(buf, "WiFi Protected Setup (WPS): failed in enrollee mode", sizeof(buf));
    break;
  case ARDUINO_EVENT_WPS_ER_TIMEOUT:
    strlcpy(buf, "WiFi Protected Setup (WPS): timeout in enrollee mode", sizeof(buf));
    break;
  case ARDUINO_EVENT_WPS_ER_PIN:
    strlcpy(buf, "WiFi Protected Setup (WPS): pin code in enrollee mode", sizeof(buf));
    break;
  case ARDUINO_EVENT_WIFI_AP_START:
  {
    wifiModeAP = true;
    strlcpy(buf, "WiFi access point started", sizeof(buf));
    break;
  }
  case ARDUINO_EVENT_WIFI_AP_STOP:
  {
    wifiModeAP = false;
    strlcpy(buf, "WiFi access point stopped", sizeof(buf));
    break;
  }
  case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
    strlcpy(buf, "Client connected", sizeof(buf));
    break;
  case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
    strlcpy(buf, "Client disconnected", sizeof(buf));
    break;
  case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
    strlcpy(buf, "Assigned IP address to client", sizeof(buf));
    break;
  case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:
    strlcpy(buf, "Received probe request", sizeof(buf));
    break;
  case ARDUINO_EVENT_WIFI_AP_GOT_IP6:
    strlcpy(buf, "AP IPv6 is preferred", sizeof(buf));
    break;
  case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
    strlcpy(buf, "STA IPv6 is preferred", sizeof(buf));
    break;
  case ARDUINO_EVENT_ETH_GOT_IP6:
    break;
  case ARDUINO_EVENT_ETH_START:
    break;
  case ARDUINO_EVENT_ETH_STOP:
    break;
  case ARDUINO_EVENT_ETH_CONNECTED:
    break;
  case ARDUINO_EVENT_ETH_DISCONNECTED:
    break;
  case ARDUINO_EVENT_ETH_GOT_IP:
    break;
  default:
    break;
  }

  D_LOG("WiFi-event: %d - %s", event, buf);
}

void setupWiFiAP()
{

  static char apname[32]{};

  snprintf(apname, sizeof(apname), "%s%02x%02x", espName, sys.getMac(4), sys.getMac(5));

  /* Soft AP network parameters */
  IPAddress apIP(192, 168, 4, 1);
  IPAddress netMsk(255, 255, 255, 0);

  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(apname, wifiConfig.appasswd);
  WiFi.enableAP(true);

  delay(500);

  /* Setup the DNS server redirecting all the domains to the apIP */
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(53, "*", apIP);

  // wifiModeAP = true;

  APmodeTimeout = millis();
  N_LOG("NET: wifi AP mode started");
}

void setupWiFigeneric()
{

  // Do not use SDK storage of SSID/WPA parameters
  WiFi.persistent(false);

  // Set wifi mode to STA for next step (clear config)
  if (!WiFi.mode(WIFI_STA))
    E_LOG("NET: Unable to set WiFi mode to STA");

  // Clear any saved wifi config
  esp_err_t wifi_restore = esp_wifi_restore();
  D_LOG("NET: esp_wifi_restore: %s", esp_err_to_name(wifi_restore));

  // Disconnect any existing connections and clear
  if (!WiFi.disconnect(true, true))
    W_LOG("NET: unable to set wifi disconnect");

  // Reset wifi mode to NULL
  if (!WiFi.mode(WIFI_MODE_NULL))
    W_LOG("NET: unable to set WiFi mode to NULL");

  // Set hostname
  bool setHostname_result = WiFi.setHostname(hostName());
  D_LOG("NET: WiFi.setHostname: %s", setHostname_result ? "OK" : "NOK");

  // No AutoReconnect
  if (!WiFi.setAutoReconnect(false))
    W_LOG("NET: unable to set auto reconnect");

  delay(200);

  // Begin init of actual wifi connection

  // Set correct mode
  if (wifiConfig.aptimeout > 0)
  {
    if (WiFi.mode(WIFI_AP_STA))
      I_LOG("NET: WiFi mode WIFI_AP_STA");
  }
  else
  {
    if (WiFi.mode(WIFI_STA))
      I_LOG("NET: WiFi mode WIFI_STA");
  }

  esp_err_t esp_wifi_set_max_tx_power(int8_t power);
  esp_err_t wifi_set_max_tx_power = esp_wifi_set_max_tx_power(78);
  D_LOG("NET: esp_wifi_set_max_tx_power: %s", esp_err_to_name(wifi_set_max_tx_power));

  int8_t wifi_power_level = -1;
  esp_err_t wifi_get_max_tx_power = esp_wifi_get_max_tx_power(&wifi_power_level);
  D_LOG("NET: esp_wifi_get_max_tx_power: %s - level:%d", esp_err_to_name(wifi_get_max_tx_power), wifi_power_level);

  // Do not use flash storage for wifi settings
  esp_err_t wifi_set_storage = esp_wifi_set_storage(WIFI_STORAGE_RAM);
  D_LOG("NET: esp_wifi_set_storage: %s", esp_err_to_name(wifi_set_storage));

  // No power saving
  esp_err_t wifi_set_ps = esp_wifi_set_ps(WIFI_PS_NONE);
  D_LOG("NET: esp_wifi_set_ps: %s", esp_err_to_name(wifi_set_ps));
}

bool connectWiFiSTA(bool restore)
{
  wifiSTAconnecting = true;
  // Switch from defualt WIFI_FAST_SCAN ScanMethod to WIFI_ALL_CHANNEL_SCAN. Also set sortMethod to WIFI_CONNECT_AP_BY_SIGNAL just to be sure.
  // WIFI_FAST_SCAN will connect to the first SSID match found causing connection issues when multiple AP active with the same SSID name
  WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);
  WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);

  if (strcmp(wifiConfig.dhcp, "off") == 0)
  {
    set_static_ip_config();
  }

  delay(2000);

  // wifiModeAP = false;
  D_LOG("NET: Connecting to wireless network...");

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
      wifiSTAconnecting = false;
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

  E_LOG("NET: error - could not connect to wifi - %s", wifiConfig.wl_status_to_name(status));
  wifiSTAconnecting = false;
  wifiSTAshouldConnect = true;
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
      E_LOG("NET: error - static IP config NOK");
    }
    else
    {
      I_LOG("NET: static IP config OK");
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
      N_LOG("SYS: original SHT30 sensor found");
    }
    else if (SHT3x_alternative)
    {
      N_LOG("SYS: alternative SHT30 sensor found");
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
      W_LOG("SYS: original SHT30 sensor found (2nd try)");
    }
    else if (SHT3x_alternative)
    {
      W_LOG("SYS: alternative SHT30 sensor found (2nd try)");
    }
    else
    {
      systemConfig.syssht30 = 0;
      N_LOG("SYS: SHT30 sensor not found");
    }
  }
}

void init_vRemote()
{
  // setup virtual remote
  I_LOG("SYS: Virtual remotes, start ID: %02X,%02X,%02X - No.: %d", sys.getMac(3), sys.getMac(4), sys.getMac(5), systemConfig.itho_numvrem);

  virtualRemotes.setMaxRemotes(systemConfig.itho_numvrem);
  VirtualRemotesConfigLoaded = loadVirtualRemotesConfig("flash");
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
      // sendCO2init();
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
  bool updatefanstatus = false;

  if (origin == WEB)
    updateweb = true;

  if (strcmp(command, "away") == 0)
  {
    i2c_queue_add_cmd([remoteIndex]()
                      { sendRemoteCmd(remoteIndex, IthoAway); });
    updatefanstatus = true;
  }
  else if (strcmp(command, "low") == 0)
  {
    i2c_queue_add_cmd([remoteIndex]()
                      { sendRemoteCmd(remoteIndex, IthoLow); });
    updatefanstatus = true;
  }
  else if (strcmp(command, "medium") == 0)
  {
    i2c_queue_add_cmd([remoteIndex]()
                      { sendRemoteCmd(remoteIndex, IthoMedium); });
    updatefanstatus = true;
  }
  else if (strcmp(command, "high") == 0)
  {
    i2c_queue_add_cmd([remoteIndex]()
                      { sendRemoteCmd(remoteIndex, IthoHigh); });
    updatefanstatus = true;
  }
  else if (strcmp(command, "timer1") == 0)
  {
    i2c_queue_add_cmd([remoteIndex]()
                      { sendRemoteCmd(remoteIndex, IthoTimer1); });
    updatefanstatus = true;
  }
  else if (strcmp(command, "timer2") == 0)
  {
    i2c_queue_add_cmd([remoteIndex]()
                      { sendRemoteCmd(remoteIndex, IthoTimer2); });
    updatefanstatus = true;
  }
  else if (strcmp(command, "timer3") == 0)
  {
    i2c_queue_add_cmd([remoteIndex]()
                      { sendRemoteCmd(remoteIndex, IthoTimer3); });
    updatefanstatus = true;
  }
  else if (strcmp(command, "cook30") == 0)
  {
    i2c_queue_add_cmd([remoteIndex]()
                      { sendRemoteCmd(remoteIndex, IthoCook30); });
    updatefanstatus = true;
  }
  else if (strcmp(command, "cook60") == 0)
  {
    i2c_queue_add_cmd([remoteIndex]()
                      { sendRemoteCmd(remoteIndex, IthoCook60); });
    updatefanstatus = true;
  }
  else if (strcmp(command, "auto") == 0)
  {
    i2c_queue_add_cmd([remoteIndex]()
                      { sendRemoteCmd(remoteIndex, IthoAuto); });
    updatefanstatus = true;
  }
  else if (strcmp(command, "autonight") == 0)
  {
    i2c_queue_add_cmd([remoteIndex]()
                      { sendRemoteCmd(remoteIndex, IthoAutoNight); });
    updatefanstatus = true;
  }
  else if (strcmp(command, "join") == 0)
  {
    i2c_queue_add_cmd([remoteIndex]()
                      { sendRemoteCmd(remoteIndex, IthoJoin); });
    updatefanstatus = true;
  }
  else if (strcmp(command, "leave") == 0)
  {
    i2c_queue_add_cmd([remoteIndex]()
                      { sendRemoteCmd(remoteIndex, IthoLeave); });
    updatefanstatus = true;
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

  if (updatefanstatus)
  {
    i2c_queue_add_cmd([]()
                      { sendQuery31DA(false); });
    i2c_queue_add_cmd([]()
                      { sendQuery31D9(false); });
    i2c_queue_add_cmd([]()
                      { updateMQTTihtoStatus = true; });
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
