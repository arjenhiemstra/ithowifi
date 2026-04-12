

#include "tasks/task_syscontrol.h"
#include "../sys_log.h"
#include "generic_functions.h"

#define TASK_SYS_CONTROL_PRIO 6
// globals
uint32_t TaskSysControlHWmark = 0;
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
bool restest = false;
bool i2c_safe_guard_log = true;
unsigned long getFWupdateInfo = 0;
struct firmwareinfo firmwareInfo;

// locals
StaticTask_t xTaskSysControlBuffer;
StackType_t xTaskSysControlStack[STACK_SIZE_MEDIUM];
TaskHandle_t xTaskSysControlHandle = NULL;

Ticker scan;

unsigned long lastWIFIReconnectAttempt = 0;
unsigned long wifiLedUpdate = 0;
unsigned long SHT3x_readout = 0;
unsigned long query2401tim = 0;
unsigned long query4210tim = 0;
unsigned long mqttUpdatetim = 0;
unsigned long lastVersionCheck = 0;
unsigned long lastIthoStatusFormatCheck = 0;

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

void TaskSysControl(void *pvParameters)
{
  configASSERT((uint32_t)pvParameters == 1UL);
  Ticker TaskTimeout;
  Ticker queueUpdater;

  networkManager.initialize(); // Initialize network clients (sets secure client to skip CA verification)

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

  queueUpdater.attach_ms(QUEUE_UPDATE_MS, updateQueue);

  for (;;)
  {
    yield();
    esp_task_wdt_reset();

    TaskTimeout.once_ms(TASK_SYSCONTROL_TIMEOUT_MS, []()
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
  if (otaUpdateRequested)
  {
    otaUpdateRequested = false;
    if (otaUpdateURL[0] != '\0')
      performOTAUpdateFromURL(otaUpdateURL);
    else
      performOTAUpdate(otaUpdateBeta);
  }

  i2cManager.processQueue();

  if (systemConfig.itho_rf_standalone == 1)
  {
    // RF standalone mode: skip I2C, mark init done
    if (!i2c_init_functions_done)
    {
      i2c_init_functions_done = true;
      ithoInitResult = 2; // 2 = RF standalone (not I2C), avoids low-power TX path
      I_LOG("SYS: RF standalone mode - I2C disabled");
      sendHomeAssistantDiscovery = true;
    }
  }
  else
  {
    if (IthoInit && millis() > 250)
    {
      IthoInit = ithoInitCheck();
    }

    if (!i2c_init_functions_done)
    {
      initI2cFunctions();
      if (i2c_init_functions_done)
      {
        // Reset timers so normal intervals start from init completion
        query4210tim = millis();
        query2401tim = millis();
      }
    }
    // request itho counter status every systemConfig.itho_counter_updatefreq sec.
    else if (millis() - query4210tim >= systemConfig.itho_counter_updatefreq * 1000UL)
    {
      query4210tim = millis();
      if (systemConfig.itho_4210 == 1)
      {
        i2cQueueAddCmd([]()
                          { sendQueryCounters(false); });
        i2cQueueAddCmd([]()
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
        i2cQueueAddCmd([]()
                          { sendQueryStatus(false); });
      }
      if (systemConfig.itho_31da == 1)
      {
        _updateMQTT = true;
        i2cQueueAddCmd([]()
                          { sendQuery31DA(false); });
      }
      if (systemConfig.itho_31d9 == 1)
      {
        _updateMQTT = true;
        i2cQueueAddCmd([]()
                          { sendQuery31D9(false); });
      }
      if (_updateMQTT)
      {
        i2cQueueAddCmd([]()
                          { updateMQTTihtoStatus = true; });
      }
    }

    if (systemConfig.itho_sendjoin > 0 && !joinSend && ithoInitResult == 1)
    {
      joinSend = true;
      i2cQueueAddCmd([]()
                        { sendRemoteCmd(0, IthoJoin); });

      I_LOG("I2C init: Virtual remote join command send");

      if (systemConfig.itho_sendjoin == 1)
      {
        systemConfig.itho_sendjoin = 0;
        saveSystemConfig("flash");
      }
    }

    // Itho queue (I2C PWM control)
    if (clearQueue)
    {
      clearQueue = false;
      ithoQueue.clearQueue();
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
  if (WiFi.status() != WL_CONNECTED && wifiSTAshouldConnect && !wifiSTAconnecting)
  {
    if (digitalRead(hardwareManager.wifi_led_pin) == LOW)
    {
      digitalWrite(hardwareManager.wifi_led_pin, HIGH);
    }
    if (millis() - lastWIFIReconnectAttempt > WIFI_RECONNECT_INTERVAL_MS)
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
    if (digitalRead(hardwareManager.wifi_led_pin) == HIGH)
    {
      digitalWrite(hardwareManager.wifi_led_pin, LOW);
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
    E_LOG("I2C: error - StatusFormat lost. Resend query result: %s", ithoStatus.size() ? "success" : "failed");
  }

  if (!(currentItho_fwversion() > 0) && millis() - lastVersionCheck > 60000)
  {
    lastVersionCheck = millis();
    i2cQueueAddCmd([]()
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
        i2cQueueAddCmd([]()
                          { updateShtSensor(); });
      }
    }
  }
  if (reset_sht_sensor && (!SHT3x_alternative && !SHT3x_original))
  {
    reset_sht_sensor = false;
    triggerShtSensorReset();
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
    checkFirmwareUpdate();
  }
}

void updateQueue()
{
  ithoQueue.updateQueue();
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

    i2cQueueAddCmd([value, ithoCurrentVal, updateIthoMQTT]()
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
    i2cQueueAddCmd([remoteIndex]()
                      { sendRemoteCmd(remoteIndex, IthoAway); });
    updatefanstatus = true;
  }
  else if (strcmp(command, "low") == 0)
  {
    i2cQueueAddCmd([remoteIndex]()
                      { sendRemoteCmd(remoteIndex, IthoLow); });
    updatefanstatus = true;
  }
  else if (strcmp(command, "medium") == 0)
  {
    i2cQueueAddCmd([remoteIndex]()
                      { sendRemoteCmd(remoteIndex, IthoMedium); });
    updatefanstatus = true;
  }
  else if (strcmp(command, "high") == 0)
  {
    i2cQueueAddCmd([remoteIndex]()
                      { sendRemoteCmd(remoteIndex, IthoHigh); });
    updatefanstatus = true;
  }
  else if (strcmp(command, "timer1") == 0)
  {
    i2cQueueAddCmd([remoteIndex]()
                      { sendRemoteCmd(remoteIndex, IthoTimer1); });
    updatefanstatus = true;
  }
  else if (strcmp(command, "timer2") == 0)
  {
    i2cQueueAddCmd([remoteIndex]()
                      { sendRemoteCmd(remoteIndex, IthoTimer2); });
    updatefanstatus = true;
  }
  else if (strcmp(command, "timer3") == 0)
  {
    i2cQueueAddCmd([remoteIndex]()
                      { sendRemoteCmd(remoteIndex, IthoTimer3); });
    updatefanstatus = true;
  }
  else if (strcmp(command, "cook30") == 0)
  {
    i2cQueueAddCmd([remoteIndex]()
                      { sendRemoteCmd(remoteIndex, IthoCook30); });
    updatefanstatus = true;
  }
  else if (strcmp(command, "cook60") == 0)
  {
    i2cQueueAddCmd([remoteIndex]()
                      { sendRemoteCmd(remoteIndex, IthoCook60); });
    updatefanstatus = true;
  }
  else if (strcmp(command, "auto") == 0)
  {
    i2cQueueAddCmd([remoteIndex]()
                      { sendRemoteCmd(remoteIndex, IthoAuto); });
    updatefanstatus = true;
  }
  else if (strcmp(command, "autonight") == 0)
  {
    i2cQueueAddCmd([remoteIndex]()
                      { sendRemoteCmd(remoteIndex, IthoAutoNight); });
    updatefanstatus = true;
  }
  else if (strcmp(command, "join") == 0)
  {
    i2cQueueAddCmd([remoteIndex]()
                      { sendRemoteCmd(remoteIndex, IthoJoin); });
    updatefanstatus = true;
  }
  else if (strcmp(command, "leave") == 0)
  {
    i2cQueueAddCmd([remoteIndex]()
                      { sendRemoteCmd(remoteIndex, IthoLeave); });
    updatefanstatus = true;
  }
  else if (strcmp(command, "type") == 0)
  {
    i2cQueueAddCmd([updateweb]()
                      { sendQueryDevicetype(updateweb); });
  }
  else if (strcmp(command, "status") == 0)
  {
    i2cQueueAddCmd([updateweb]()
                      { sendQueryStatus(updateweb); });
  }
  else if (strcmp(command, "statusformat") == 0)
  {
    i2cQueueAddCmd([updateweb]()
                      { sendQueryStatusFormat(updateweb); });
  }
  else if (strcmp(command, "31DA") == 0)
  {
    i2cQueueAddCmd([updateweb]()
                      { sendQuery31DA(updateweb); });
  }
  else if (strcmp(command, "31D9") == 0)
  {
    i2cQueueAddCmd([updateweb]()
                      { sendQuery31D9(updateweb); });
  }
  else if (strcmp(command, "10D0") == 0)
  {
    i2cQueueAddCmd([]()
                      { filterReset(); });
  }
  else if (strcmp(command, "shtreset") == 0)
  {
    reset_sht_sensor = true;
  }

  if (updatefanstatus)
  {
    i2cQueueAddCmd([]()
                      { sendQuery31DA(false); });
    i2cQueueAddCmd([]()
                      { sendQuery31D9(false); });
    i2cQueueAddCmd([]()
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
