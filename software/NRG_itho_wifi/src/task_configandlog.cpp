#include "task_configandlog.h"

#define TASK_CONFIG_AND_LOG_PRIO 5

// globals
Ticker TaskConfigAndLogTimeout;
TaskHandle_t xTaskConfigAndLogHandle = NULL;
uint32_t TaskConfigAndLogHWmark = 0;
volatile bool saveRemotesflag = false;
volatile bool saveVremotesflag = false;
bool formatFileSystem = false;

// locals
StaticTask_t xTaskConfigAndLogBuffer;
StackType_t xTaskConfigAndLog[STACK_SIZE];

Ticker DelayedSave;

unsigned long lastLog = 0;

void startTaskConfigAndLog()
{
  xTaskConfigAndLogHandle = xTaskCreateStaticPinnedToCore(
      TaskConfigAndLog,
      "TaskConfigAndLog",
      STACK_SIZE,
      (void *)1,
      TASK_CONFIG_AND_LOG_PRIO,
      xTaskConfigAndLog,
      &xTaskConfigAndLogBuffer,
      CONFIG_ARDUINO_RUNNING_CORE);
}

void TaskConfigAndLog(void *pvParameters)
{
  configASSERT((uint32_t)pvParameters == 1UL);

  initFileSystem();
  logInit();
  loadSystemConfig();

  startTaskSysControl();

  esp_task_wdt_add(NULL);

  for (;;)
  {
    yield();
    esp_task_wdt_reset();

    TaskConfigAndLogTimeout.once_ms(3000, []()
                                    { logInput("Warning: Task ConfigAndLog timed out!"); });

    execLogAndConfigTasks();

    TaskConfigAndLogHWmark = uxTaskGetStackHighWaterMark(NULL);
    vTaskDelay(25 / portTICK_PERIOD_MS);
  }

  // else delete task
  vTaskDelete(NULL);
}

void execLogAndConfigTasks()
{
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
    char logBuff[LOG_BUF_SIZE] = "";
    sprintf(logBuff, "Mem free: %d, Mem low: %d, Mem block: %d", sys.getMemHigh(), sys.getMemLow(), sys.getMaxFreeBlockSize());
    logInput(logBuff);

    lastLog = millis();
  }

  if (formatFileSystem)
  {
    formatFileSystem = false;
    char logBuff[LOG_BUF_SIZE] = "";
    StaticJsonDocument<128> root;
    JsonObject systemstat = root.createNestedObject("systemstat");

    if (ACTIVE_FS.format())
    {
      systemstat["format"] = 1;
      notifyClients(root.as<JsonObjectConst>());
      strcpy(logBuff, "Filesystem format done");
      logMessagejson(logBuff, WEBINTERFACE);
      strcpy(logBuff, "Device rebooting, connect to accesspoint to setup the device");
      logMessagejson(logBuff, WEBINTERFACE);
      esp_task_wdt_init(1, true);
      esp_task_wdt_add(NULL);
      while (true)
        ;
    }
    else
    {
      systemstat["format"] = 0;
      strcpy(logBuff, "Unable to format");
      logMessagejson(logBuff, WEBINTERFACE);
      notifyClients(root.as<JsonObjectConst>());
    }
    strcpy(logBuff, "");
  }
}

bool initFileSystem()
{

  D_LOG("Mounting FS...\n");

  ACTIVE_FS.begin(true);

  return true;
}

void logInit()
{
  filePrint.open();

  Log.begin(LOG_LEVEL_NOTICE, &filePrint);
  Log.setPrefix(printTimestamp);
  Log.setSuffix(printNewline);

  filePrint.close();

  delay(100);
  char logBuff[LOG_BUF_SIZE] = "";

  uint8_t reason = esp_reset_reason();
  char buf[32] = "";

  switch (reason)
  {
  case 1:
    strcpy(buf, "POWERON_RESET");
    break; /**<1,  Vbat power on reset*/
  case 3:
    strcpy(buf, "SW_RESET");
    break; /**<3,  Software reset digital core*/
  case 4:
    strcpy(buf, "OWDT_RESET");
    break; /**<4,  Legacy watch dog reset digital core*/
  case 5:
    strcpy(buf, "DEEPSLEEP_RESET");
    break; /**<5,  Deep Sleep reset digital core*/
  case 6:
    strcpy(buf, "SDIO_RESET");
    break; /**<6,  Reset by SLC module, reset digital core*/
  case 7:
    strcpy(buf, "TG0WDT_SYS_RESET");
    break; /**<7,  Timer Group0 Watch dog reset digital core*/
  case 8:
    strcpy(buf, "TG1WDT_SYS_RESET");
    break; /**<8,  Timer Group1 Watch dog reset digital core*/
  case 9:
    strcpy(buf, "RTCWDT_SYS_RESET");
    break; /**<9,  RTC Watch dog Reset digital core*/
  case 10:
    strcpy(buf, "INTRUSION_RESET");
    break; /**<10, Instrusion tested to reset CPU*/
  case 11:
    strcpy(buf, "TGWDT_CPU_RESET");
    break; /**<11, Time Group reset CPU*/
  case 12:
    strcpy(buf, "SW_CPU_RESET");
    break; /**<12, Software reset CPU*/
  case 13:
    strcpy(buf, "RTCWDT_CPU_RESET");
    break; /**<13, RTC Watch dog Reset CPU*/
  case 14:
    strcpy(buf, "EXT_CPU_RESET");
    break; /**<14, for APP CPU, reseted by PRO CPU*/
  case 15:
    strcpy(buf, "RTCWDT_BROWN_OUT_RESET");
    break; /**<15, Reset when the vdd voltage is not stable*/
  case 16:
    strcpy(buf, "RTCWDT_RTC_RESET");
    break; /**<16, RTC Watch dog reset digital core and rtc module*/
  default:
    strcpy(buf, "NO_MEAN");
  }
  sprintf(logBuff, "System boot, last reset reason: %s", buf);

  logInput(logBuff);

  strcpy(logBuff, "");
  sprintf(logBuff, "HW rev: %s, FW ver.: %s", HWREVISION, FWVERSION);
  logInput(logBuff);
}
