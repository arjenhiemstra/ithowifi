#include "tasks/task_configandlog.h"

#define TASK_CONFIG_AND_LOG_PRIO 5

// globals
Ticker TaskConfigAndLogTimeout;
TaskHandle_t xTaskConfigAndLogHandle = NULL;
uint32_t TaskConfigAndLogHWmark = 0;
volatile bool saveRemotesflag = false;
volatile bool saveVremotesflag = false;
bool chkpartition = false;
int chk_partition_res = -1;
bool formatFileSystem = false;
bool flashLogInitReady = false;
char uuid[UUID_STR_LEN]{};

// bool RemotesConfigLoaded = false;
// bool VirtualRemotesConfigLoaded = false;
// bool WifiConfigLoaded = false;
// bool SystemConfigLoaded = false;
// bool logConfigLoaded = false;
// bool HADiscConfigLoaded = false;

// locals
FSFilePrint filePrint(ACTIVE_FS, "/logfile", 2, 10000);

StaticTask_t xTaskConfigAndLogBuffer;
StackType_t xTaskConfigAndLog[STACK_SIZE_MEDIUM];

Ticker DelayedSave;

unsigned long lastLog = 0;

void startTaskConfigAndLog()
{
  xTaskConfigAndLogHandle = xTaskCreateStaticPinnedToCore(
      TaskConfigAndLog,
      "TaskConfigAndLog",
      STACK_SIZE_MEDIUM,
      (void *)1,
      TASK_CONFIG_AND_LOG_PRIO,
      xTaskConfigAndLog,
      &xTaskConfigAndLogBuffer,
      CONFIG_ARDUINO_RUNNING_CORE);
}

void TaskConfigAndLog(void *pvParameters)
{
  configASSERT((uint32_t)pvParameters == 1UL);

  syslog_queueSemaphore = xSemaphoreCreateBinary();
  xSemaphoreGive(syslog_queueSemaphore);

  syslog_queue_worker();

  initFileSystem();
  syslog_queue_worker();

  // set provisional timezone
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();

  logInit();
  syslog_queue_worker();

  logConfigLoaded = loadLogConfig("flash");
  syslog_queue_worker();
  if (logConfig.esplog_active == 1)
  {
    esp_log_level_set("*", ESP_LOG_ERROR);
    esp_log_set_vprintf(&esp_vprintf);
    log_e("loge_e test %d", 1);
  }

  SystemConfigLoaded = loadSystemConfig("flash");
  ithoQueue.set_itho_fallback_speed(systemConfig.itho_fallback);
  if (systemConfig.fw_check)
    getFWupdateInfo = 25 * 60 * 60 * 1000; // trigger firmware update check after boot
  syslog_queue_worker();

  HADiscConfigLoaded = loadHADiscConfig("flash");

  startTaskSysControl();
  syslog_queue_worker();

  esp_task_wdt_add(NULL);

  for (;;)
  {
    yield();
    esp_task_wdt_reset();

    TaskConfigAndLogTimeout.once_ms(15000, []()
                                    { W_LOG("SYS: warning - Task ConfigAndLog timed out!"); });

    execLogAndConfigTasks();

    TaskConfigAndLogHWmark = uxTaskGetStackHighWaterMark(NULL);
    vTaskDelay(25 / portTICK_PERIOD_MS);
  }

  // else delete task
  TaskHandle_t xTempTask = xTaskConfigAndLogHandle;
  xTaskConfigAndLogHandle = NULL;
  vTaskDelete(xTempTask);
}

void execLogAndConfigTasks()
{
  syslog_queue_worker();
  // Logging and config tasks
  if (saveSystemConfigflag && SystemConfigLoaded)
  {
    saveSystemConfigflag = false;
    if (saveSystemConfig("flash"))
    {
      logMessagejson("System settings saved", WEBINTERFACE);
    }
    else
    {
      logMessagejson("Failed saving System settings: Unable to write config file", WEBINTERFACE);
    }
  }
  if (saveLogConfigflag && logConfigLoaded)
  {
    saveLogConfigflag = false;
    if (saveLogConfig("flash"))
    {
      logMessagejson("Log settings saved", WEBINTERFACE);
    }
    else
    {
      logMessagejson("Failed saving Log settings: Unable to write config file", WEBINTERFACE);
    }
  }
  if (saveHADiscConfigflag && HADiscConfigLoaded)
  {
    saveHADiscConfigflag = false;
    if (saveHADiscConfig("flash"))
    {
      logMessagejson("HA Discovery settings saved", WEBINTERFACE);
      sendHomeAssistantDiscovery = true;
    }
    else
    {
      logMessagejson("Failed saving HA Discovery settings: Unable to write config file", WEBINTERFACE);
    }
  }
  if (saveWifiConfigflag && WifiConfigLoaded)
  {
    saveWifiConfigflag = false;

    bool reconnect = false;
    JsonDocument root;
    File configFile = ACTIVE_FS.open("/wifi.json", "r");
    if (configFile)
    {
      DeserializationError error = deserializeJson(root, configFile);
      if (!error)
      {
        if ((strcmp(root["ssid"].as<const char *>(), wifiConfig.ssid) != 0 && strcmp(root["ssid"].as<const char *>(), "") != 0) || (strcmp(root["passwd"].as<const char *>(), wifiConfig.passwd) != 0))
        {
          reconnect = true;
        }
      }
    }

    if (saveWifiConfig("flash"))
    {

      if (reconnect)
      {
        char buf[128]{};
        snprintf(buf, sizeof(buf), "Wifi settings changed, connecting to %s", wifiConfig.ssid);
        logMessagejson(buf, WEBINTERFACE);
        connectWiFiSTA();
      }
      else
      {
        logMessagejson("Wifi settings saved...", WEBINTERFACE);
      }
    }
    else
    {
      logMessagejson("Failed saving Wifi settings: Unable to write config file", WEBINTERFACE);
    }
  }

  if (saveRemotesflag && RemotesConfigLoaded)
  {
    saveRemotesflag = false;
    DelayedSave.once_ms(150, []()
                        {
      saveRemotesConfig("flash");
      jsonWsSend("remotes"); });
    logMessagejson("RF Config saved", WEBINTERFACE);
  }
  if (saveVremotesflag && VirtualRemotesConfigLoaded)
  {
    saveVremotesflag = false;
    DelayedSave.once_ms(150, []()
                        {
      saveVirtualRemotesConfig("flash");
      jsonWsSend("vremotes"); });
    logMessagejson("Virtual remotes config saved", WEBINTERFACE);
  }
  if (resetWifiConfigflag)
  {
    resetWifiConfigflag = false;
    if (resetWifiConfig())
    {
      logMessagejson("Wifi settings restored, reboot the device", WEBINTERFACE);
    }
    else
    {
      logMessagejson("Failed restoring Wifi settings, please try again", WEBINTERFACE);
    }
  }
  if (resetSystemConfigflag)
  {
    resetSystemConfigflag = false;

    if (resetSystemConfigs())
    {
      logMessagejson("System settings restored, reboot the device", WEBINTERFACE);
    }
    else
    {
      logMessagejson("Failed restoring System settings, please try again", WEBINTERFACE);
    }
  }
  if (saveAllConfigsflag)
  {
    saveAllConfigsflag = false;

    if (saveSystemConfigs())
    {
      logMessagejson("All system config files saved", WEBINTERFACE);
    }
    else
    {
      logMessagejson("Failed saving system config files, please try again", WEBINTERFACE);
    }
  }

  if (millis() - lastLog > LOGGING_INTERVAL)
  {
    log_mem_info();
    lastLog = millis();
  }

  if (formatFileSystem)
  {
    formatFileSystem = false;
    char logBuff[LOG_BUF_SIZE]{};
    JsonDocument root;
    JsonObject systemstat = root["systemstat"].to<JsonObject>();

    if (ACTIVE_FS.format())
    {
      systemstat["format"] = 1;
      notifyClients(root.as<JsonObject>());
      strlcpy(logBuff, "Filesystem format done", sizeof(logBuff));
      logMessagejson(logBuff, WEBINTERFACE);
      strlcpy(logBuff, "Device rebooting, connect to accesspoint to setup the device", sizeof(logBuff));
      logMessagejson(logBuff, WEBINTERFACE);
      esp_restart();
    }
    else
    {
      systemstat["format"] = 0;
      strlcpy(logBuff, "Unable to format", sizeof(logBuff));
      logMessagejson(logBuff, WEBINTERFACE);
      notifyClients(root.as<JsonObject>());
    }
    strlcpy(logBuff, "", sizeof(logBuff));
  }
  if (chkpartition)
  {
    chkpartition = false;
    chk_partition_res = current_partition_scheme();
    if (chk_partition_res == -1)
    {
      D_LOG("SYS: chkpartition error");
    }
    else
    {
      jsonWsSend("chkpart");
    }
  }
}

void syslog_queue_worker()
{

  while (!syslog_queue.empty())
  {
    log_msg input = syslog_queue.front();
    syslog_queue.pop_front();

    if (flashLogInitReady)
    {
      // yield();

      if (input.code <= SYSLOG_CRIT && logConfig.loglevel >= SYSLOG_CRIT)
      {
        filePrint.open();
        Log.fatalln(input.msg.c_str());
        filePrint.close();
      }
      else if (input.code == SYSLOG_ERR && logConfig.loglevel >= SYSLOG_ERR)
      {
        filePrint.open();
        Log.errorln(input.msg.c_str());
        filePrint.close();
      }
      else if (input.code == SYSLOG_WARNING && logConfig.loglevel >= SYSLOG_WARNING)
      {
        filePrint.open();
        Log.warningln(input.msg.c_str());
        filePrint.close();
      }
      else if (input.code == SYSLOG_NOTICE && logConfig.loglevel >= SYSLOG_NOTICE)
      {
        filePrint.open();
        Log.noticeln(input.msg.c_str());
        filePrint.close();
      }
      else if (input.code == SYSLOG_INFO && logConfig.loglevel >= SYSLOG_INFO)
      {
        filePrint.open();
        Log.infoln(input.msg.c_str());
        filePrint.close();
      }
      // Do not log DEBUG level to Flash to prevent excessive wear
      // else if (input.code == SYSLOG_DEBUG && logConfig.loglevel >= SYSLOG_DEBUG)
      // {
      //   Log.verboseln(input.msg.c_str());
      // }
      else if (input.code == ESP_SYSLOG_ERR)
      {
        Log.setPrefix(LogPrefixESP);
        Log.setShowLevel(false);
        // Log.clearSuffix();
        filePrint.open();
        Log.noticeln(input.msg.c_str());
        filePrint.close();
        Log.setPrefix(printTimestamp);
        Log.setShowLevel(true);
        // Log.setSuffix(printNewline);
      }
    }
    if (WiFi.status() == WL_CONNECTED && logConfig.syslog_active == 1)
    {
      syslog.log(input.code, input.msg.c_str());
    }
    if (WiFi.status() == WL_CONNECTED && logConfig.webserial_active && webSerial != nullptr)
    {
      webSerial->print(input.msg.c_str());
    }

    // Also update webinterface
    // JsonDocument root;
    // root["dblog"] = inputString;
    // notifyClients(root.as<JsonObject>());

    // do something
  }
}

bool initFileSystem()
{

  D_LOG("SYS: Mounting FS...");

  NVS.begin();

  // NVS.erase("uuid");
  std::string uuidstr = NVS.getstring("uuid");
  if (uuidstr.empty())
  {
    uuid_t uu;

    uuid_generate(uu);
    uuid_unparse(uu, uuid);
    NVS.setString("uuid", uuid);
  }
  else
  {
    strlcpy(uuid, uuidstr.c_str(), sizeof(uuid));
  }

  ACTIVE_FS.begin(true);

  check_partition_tables();

  return true;
}

void logInit()
{
  // log_msg

  filePrint.open();

  Log.begin(LOG_LEVEL_NOTICE, &filePrint);
  Log.setPrefix(printTimestamp);
  Log.setSuffix(printNewline);

  filePrint.close();

  delay(100);

  flashLogInitReady = true;

  uint8_t reason = esp_reset_reason();
  char buf[32]{};

  switch (reason)
  {
  case 1:
    strlcpy(buf, "POWERON_RESET", sizeof(buf));
    break; /**<1,  Vbat power on reset*/
  case 3:
    strlcpy(buf, "SW_RESET", sizeof(buf));
    break; /**<3,  Software reset digital core*/
  case 4:
    strlcpy(buf, "OWDT_RESET", sizeof(buf));
    break; /**<4,  Legacy watch dog reset digital core*/
  case 5:
    strlcpy(buf, "DEEPSLEEP_RESET", sizeof(buf));
    break; /**<5,  Deep Sleep reset digital core*/
  case 6:
    strlcpy(buf, "SDIO_RESET", sizeof(buf));
    break; /**<6,  Reset by SLC module, reset digital core*/
  case 7:
    strlcpy(buf, "TG0WDT_SYS_RESET", sizeof(buf));
    break; /**<7,  Timer Group0 Watch dog reset digital core*/
  case 8:
    strlcpy(buf, "TG1WDT_SYS_RESET", sizeof(buf));
    break; /**<8,  Timer Group1 Watch dog reset digital core*/
  case 9:
    strlcpy(buf, "RTCWDT_SYS_RESET", sizeof(buf));
    break; /**<9,  RTC Watch dog Reset digital core*/
  case 10:
    strlcpy(buf, "INTRUSION_RESET", sizeof(buf));
    break; /**<10, Instrusion tested to reset CPU*/
  case 11:
    strlcpy(buf, "TGWDT_CPU_RESET", sizeof(buf));
    break; /**<11, Time Group reset CPU*/
  case 12:
    strlcpy(buf, "SW_CPU_RESET", sizeof(buf));
    break; /**<12, Software reset CPU*/
  case 13:
    strlcpy(buf, "RTCWDT_CPU_RESET", sizeof(buf));
    break; /**<13, RTC Watch dog Reset CPU*/
  case 14:
    strlcpy(buf, "EXT_CPU_RESET", sizeof(buf));
    break; /**<14, for APP CPU, reseted by PRO CPU*/
  case 15:
    strlcpy(buf, "RTCWDT_BROWN_OUT_RESET", sizeof(buf));
    break; /**<15, Reset when the vdd voltage is not stable*/
  case 16:
    strlcpy(buf, "RTCWDT_RTC_RESET", sizeof(buf));
    break; /**<16, RTC Watch dog reset digital core and rtc module*/
  default:
    strlcpy(buf, "NO_MEAN", sizeof(buf));
  }

  N_LOG("SYS: last reset reason: %s", buf);

  N_LOG("SYS: device UUID: %s", uuid);

  I_LOG("SYS: detected hardware: 0x%02X", hardware_rev_det);

  N_LOG("SYS: hw rev: %s, fw ver.: %s", hw_revision, fw_version);

  N_LOG("I2C: sniffer capable hardware: %s", i2c_sniffer_capable ? "yes" : "no");

  D_LOG("I2C: master pins - SDA: %d SCL: %d", master_sda_pin, master_scl_pin);

  D_LOG("I2C: slave pins - SDA: %d SCL: %d", slave_sda_pin, slave_scl_pin);

  if (i2c_sniffer_capable)
  {
    D_LOG("I2C: sniffer pins - SDA: %d SCL: %d", sniffer_sda_pin, sniffer_scl_pin);
    D_LOG("I2C: debug menu: %s", systemConfig.i2cmenu ? "on" : "off");
    D_LOG("I2C: sniffer state: %s", systemConfig.i2c_sniffer ? "on" : "off");
  }
}

void log_mem_info()
{
  I_LOG("SYS: mem free: %d, mem low: %d, mem block: %u", sys.getMemHigh(), sys.getMemLow(), sys.getMaxFreeBlockSize());
}
