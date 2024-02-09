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

  syslog_queue_worker();

  initFileSystem();
  syslog_queue_worker();

  // set provisional timezone
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();

  logInit();
  syslog_queue_worker();

  loadLogConfig("flash");
  syslog_queue_worker();
  if (logConfig.esplog_active == 1)
  {
    D_LOG("Setup: esp_vprintf on");
    esp_log_level_set("*", ESP_LOG_ERROR);
    esp_log_set_vprintf(&esp_vprintf);
    log_e("loge_e test %d", 1);
  }

  loadSystemConfig("flash");
  ithoQueue.set_itho_fallback_speed(systemConfig.itho_fallback);

  syslog_queue_worker();

  startTaskSysControl();
  syslog_queue_worker();

  esp_task_wdt_add(NULL);

  for (;;)
  {
    yield();
    esp_task_wdt_reset();

    TaskConfigAndLogTimeout.once_ms(15000, []()
                                    { W_LOG("Warning: Task ConfigAndLog timed out!"); });

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
  if (saveSystemConfigflag)
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
  if (saveLogConfigflag)
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
  if (saveWifiConfigflag)
  {
    saveWifiConfigflag = false;
    if (saveWifiConfig("flash"))
    {
      logMessagejson("Wifi settings saved, connecting to wifi network...", WEBINTERFACE);
      connectWiFiSTA();
    }
    else
    {
      logMessagejson("Failed saving Wifi settings: Unable to write config file", WEBINTERFACE);
    }
  }

  if (saveRemotesflag)
  {
    saveRemotesflag = false;
    DelayedSave.once_ms(150, []()
                        {
      saveRemotesConfig("flash");
      jsonWsSend("remotes"); });
  }
  if (saveVremotesflag)
  {
    saveVremotesflag = false;
    DelayedSave.once_ms(150, []()
                        {
      saveVirtualRemotesConfig("flash");
      jsonWsSend("vremotes"); });
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
    if (resetSystemConfig())
    {
      logMessagejson("System settings restored, reboot the device", WEBINTERFACE);
    }
    else
    {
      logMessagejson("Failed restoring System settings, please try again", WEBINTERFACE);
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
      D_LOG("chkpartition error");
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
      yield();

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
      // Do not log INFO en DEBUG levels to Flash to prevent excessive wear
      // else if (input.code == SYSLOG_INFO && logConfig.loglevel >= SYSLOG_INFO)
      // {
      //   Log.traceln(input.msg.c_str());
      // }
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

    // Also update webinterface
    // JsonDocument root;
    // root["dblog"] = inputString;
    // notifyClients(root.as<JsonObject>());

    // do something
  }
}

bool initFileSystem()
{

  D_LOG("Mounting FS...");

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
  else {
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

  flashLogInitReady = true;

  delay(100);

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

  N_LOG("System boot, last reset reason: %s", buf);

  N_LOG("Device UUID: %s", uuid);

  I_LOG("Hardware detected: 0x%02X", hardware_rev_det);

  N_LOG("HW rev: %s, FW ver.: %s", hw_revision, FWVERSION);

  N_LOG("I2C sniffer capable hardware: %s", i2c_sniffer_capable ? "yes" : "no");

  D_LOG("I2C master pins - SDA: %d SCL: %d", master_sda_pin, master_scl_pin);

  D_LOG("I2C slave pins - SDA: %d SCL: %d", slave_sda_pin, slave_scl_pin);

  if (i2c_sniffer_capable)
  {
    D_LOG("I2C sniffer pins - SDA: %d SCL: %d", sniffer_sda_pin, sniffer_scl_pin);
  }
}

void log_mem_info()
{
  I_LOG("Mem free: %d, Mem low: %d, Mem block: %u", sys.getMemHigh(), sys.getMemLow(), sys.getMaxFreeBlockSize());
}
