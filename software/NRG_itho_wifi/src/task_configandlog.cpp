#include "task_configandlog.h"

#define TASK_CONFIG_AND_LOG_PRIO 5

// globals
Ticker TaskConfigAndLogTimeout;
TaskHandle_t xTaskConfigAndLogHandle = NULL;
uint32_t TaskConfigAndLogHWmark = 0;
volatile bool saveRemotesflag = false;
volatile bool saveVremotesflag = false;
bool formatFileSystem = false;
bool flashLogInitReady = false;

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

  logInit();
  syslog_queue_worker();

  loadSystemConfig();
  syslog_queue_worker();

  loadLogConfig();
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
  vTaskDelete(NULL);
}

void execLogAndConfigTasks()
{
  syslog_queue_worker();
  // Logging and config tasks
  if (saveSystemConfigflag)
  {
    saveSystemConfigflag = false;
    if (saveSystemConfig())
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
    if (saveLogConfig())
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
    if (saveWifiConfig())
    {
      logMessagejson("Wifi settings saved, reboot the device", WEBINTERFACE);
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
      saveRemotesConfig();
      jsonWsSend("remotes"); });
  }
  if (saveVremotesflag)
  {
    saveVremotesflag = false;
    DelayedSave.once_ms(150, []()
                        {
      saveVirtualRemotesConfig();
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
    StaticJsonDocument<128> root;
    JsonObject systemstat = root.createNestedObject("systemstat");

    if (ACTIVE_FS.format())
    {
      systemstat["format"] = 1;
      notifyClients(root.as<JsonObjectConst>());
      strlcpy(logBuff, "Filesystem format done", sizeof(logBuff));
      logMessagejson(logBuff, WEBINTERFACE);
      strlcpy(logBuff, "Device rebooting, connect to accesspoint to setup the device", sizeof(logBuff));
      logMessagejson(logBuff, WEBINTERFACE);
      esp_task_wdt_init(1, true);
      esp_task_wdt_add(NULL);
      while (true)
        ;
    }
    else
    {
      systemstat["format"] = 0;
      strlcpy(logBuff, "Unable to format", sizeof(logBuff));
      logMessagejson(logBuff, WEBINTERFACE);
      notifyClients(root.as<JsonObjectConst>());
    }
    strlcpy(logBuff, "", sizeof(logBuff));
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

      filePrint.open();

      if (input.code <= SYSLOG_CRIT && logConfig.loglevel >= SYSLOG_CRIT)
      {
        Log.fatalln(input.msg.c_str());
      }
      else if (input.code == SYSLOG_ERR && logConfig.loglevel >= SYSLOG_ERR)
      {
        Log.errorln(input.msg.c_str());
      }
      else if (input.code == SYSLOG_WARNING && logConfig.loglevel >= SYSLOG_WARNING)
      {
        Log.warningln(input.msg.c_str());
      }
      else if (input.code == SYSLOG_NOTICE && logConfig.loglevel >= SYSLOG_NOTICE)
      {
        Log.noticeln(input.msg.c_str());
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

      filePrint.close();
    }
    if (WiFi.status() == WL_CONNECTED && logConfig.syslog_active == 1)
    {
      syslog.log(input.code, input.msg.c_str());
    }

    // Also update webinterface
    // DynamicJsonDocument root(250);
    // root["dblog"] = inputString;
    // notifyClients(root.as<JsonObjectConst>());

    // do something
  }
}

bool initFileSystem()
{

  D_LOG("Mounting FS...");

  ACTIVE_FS.begin(true);

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
