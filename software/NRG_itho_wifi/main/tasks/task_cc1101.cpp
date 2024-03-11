#include "tasks/task_cc1101.h"

#define TASK_CC1101_PRIO 5

// globals
Ticker TaskCC1101Timeout;
TaskHandle_t xTaskCC1101Handle = nullptr;
uint32_t TaskCC1101HWmark = 0;
uint8_t debugLevel = 0;

// locals
StaticTask_t xTaskCC1101Buffer;
StackType_t xTaskCC1101Stack[STACK_SIZE_MEDIUM];

Ticker timerLearnLeaveMode;

volatile bool ithoCheck = false;
SemaphoreHandle_t isrSemaphore;

IRAM_ATTR void ITHOinterrupt()
{
  // Try to take the semaphore
  if (xSemaphoreTakeFromISR(isrSemaphore, NULL) == pdTRUE)
  {
    // ISR code here

    ithoCheck = rf.receivePacket();

    // At the end, give back the semaphore
    xSemaphoreGiveFromISR(isrSemaphore, NULL);
  }
}

void disableRFsupport()
{
  detachInterrupt(itho_irq_pin);
}

void RFDebug(IthoPacket *packetPtr, IthoCommand cmd)
{
  char debugLog[400] = {0};
  strncat(debugLog, rf.LastMessageDecoded(packetPtr).c_str(), sizeof(debugLog) - strlen(debugLog) - 1);
  // log command
  switch (cmd)
  {
  case IthoUnknown:
    strncat(debugLog, " (cmd:unknown)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case IthoAway:
    strncat(debugLog, " (cmd:away)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case IthoLow:
    strncat(debugLog, " (cmd:low)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case IthoMedium:
    strncat(debugLog, " (cmd:medium)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case IthoHigh:
    strncat(debugLog, " (cmd:high)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case IthoFull:
    strncat(debugLog, " (cmd:full)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case IthoAuto:
    strncat(debugLog, " (cmd:auto)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case IthoAutoNight:
    strncat(debugLog, " (cmd:autonight)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case IthoTimer1:
    strncat(debugLog, " (cmd:timer1)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case IthoTimer2:
    strncat(debugLog, " (cmd:timer2)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case IthoTimer3:
    strncat(debugLog, " (cmd:timer3)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case IthoCook30:
    strncat(debugLog, " (cmd:cook30)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case IthoCook60:
    strncat(debugLog, " (cmd:cook60)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case IthoTimerUser:
    strncat(debugLog, " (cmd:timeruser)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case IthoJoin:
    strncat(debugLog, " (cmd:join)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case IthoLeave:
    strncat(debugLog, " (cmd:leave)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case IthoJoinReply:
    strncat(debugLog, " (cmd:joinreply)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case IthoPIRmotionOn:
    strncat(debugLog, " (cmd:motion_on)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case IthoPIRmotionOff:
    strncat(debugLog, " (cmd:motion_off)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  }
  D_LOG(debugLog);
  //  LogMessage.once_ms(150, []() {
  logMessagejson(debugLog, RFLOG);
  //    } );
}

void toggleRemoteLLmode(const char *remotetype)
{

  if (strcmp(remotetype, "remote") == 0)
  {
    if (virtualRemotes.remoteLearnLeaveStatus())
    {
      logMessagejson("disable virtual remote Copy ID mode first", WEBINTERFACE);
      return;
    }
    if (remotes.toggleLearnLeaveMode())
    {
      rf.setBindAllowed(true);
      timerLearnLeaveMode.attach(1, setllModeTimer);
    }
    else
    {
      rf.setBindAllowed(false);
      timerLearnLeaveMode.detach();
      remotes.setllModeTime(0);
      sysStatReq = true;
    }
  }
  if (strcmp(remotetype, "vremote") == 0)
  {
    if (remotes.remoteLearnLeaveStatus())
    {
      logMessagejson("disable remote learn/leave mode first", WEBINTERFACE);
      return;
    }
    if (virtualRemotes.toggleLearnLeaveMode())
    {
      timerLearnLeaveMode.attach(1, setllModeTimer);
    }
    else
    {
      timerLearnLeaveMode.detach();
      virtualRemotes.setllModeTime(0);
      virtualRemotes.activeRemote = -1;
      sysStatReq = true;
    }
  }
}

void setllModeTimer()
{

  if (remotes.remoteLearnLeaveStatus())
  {
    remotes.updatellModeTimer();
    sysStatReq = true;
    if (remotes.getllModeTime() == 0)
    {
      toggleRemoteLLmode("remote");
    }
    return;
  }
  if (virtualRemotes.remoteLearnLeaveStatus())
  {
    virtualRemotes.updatellModeTimer();
    sysStatReq = true;
    if (virtualRemotes.getllModeTime() == 0)
    {
      toggleRemoteLLmode("vremote");
    }
    return;
  }
}

void startTaskCC1101()
{
  xTaskCC1101Handle = xTaskCreateStaticPinnedToCore(
      TaskCC1101,
      "TaskCC1101",
      STACK_SIZE_MEDIUM,
      (void *)1,
      TASK_CC1101_PRIO,
      xTaskCC1101Stack,
      &xTaskCC1101Buffer,
      CONFIG_ARDUINO_RUNNING_CORE);
}

void TaskCC1101(void *pvParameters)
{
  D_LOG("TaskCC1101 started");
  configASSERT((uint32_t)pvParameters == 1UL);

  startTaskMQTT();

  if (systemConfig.itho_rf_support)
  {
    Ticker reboot;

    // switch off rf_support in case init fails
    systemConfig.itho_rf_support = 0;
    systemConfig.rfInitOK = false;

    // attach saveConfig and reboot script to fire after 2 sec
    reboot.attach(2, []()
                  {
      E_LOG("Setup: CC1101 RF module not found");
      saveSystemConfig("flash");
      delay(1000);
      ACTIVE_FS.end();
      esp_restart(); });

    // init the RF module
    rf.init();
    pinMode(itho_irq_pin, INPUT);
    isrSemaphore = xSemaphoreCreateBinary();
    attachInterrupt(itho_irq_pin, ITHOinterrupt, RISING);
    xSemaphoreGive(isrSemaphore);

    // this portion of code will not be reached when no RF module is present: detach reboot script, switch on rf_supprt and load remotes config
    esp_task_wdt_add(NULL);
    reboot.detach();
    N_LOG("Setup: CC1101 RF module activated");
    rf.setDeviceIDsend(sys.getMac(3), sys.getMac(4), sys.getMac(5) - 1);
    systemConfig.itho_rf_support = 1;
    loadRemotesConfig("flash");
    // rf.setBindAllowed(true);
    for (int i = 0; i < remotes.getRemoteCount(); i++)
    {
      const uint8_t *id = remotes.getRemoteIDbyIndex(i);
      rf.updateRFDeviceID(*id, *(id + 1), *(id + 2), i);
      rf.updateRFDeviceType(remotes.getRemoteType(i), i);
      rf.setRFDeviceBidirectional(i, remotes.getRemoteFunction(i) == RemoteFunctions::BIDIRECT ? true : false);
    }
    rf.setBindAllowed(false);
    rf.setAllowAll(false);

    systemConfig.rfInitOK = true;

    for (;;)
    {
      yield();
      esp_task_wdt_reset();

      TaskCC1101Timeout.once_ms(3000, []()
                                { W_LOG("Warning: CC1101 Task timed out!"); });

      if (ithoCheck)
      {
        ithoCheck = false;

        D_LOG("getpacketBufferCount: %d", rf.getpacketBufferCount());

        while (rf.getpacketBufferCount() > 0)
        {
          IthoPacket packet = rf.checkForNewPacket();
          rf.parseMessage(&packet);

          uint8_t *lastID = rf.getLastID(&packet);

          IthoCommand cmd = rf.getLastCommand(&packet);
          RemoteTypes remtype = rf.getLastRemType(&packet);
          bool chk = remotes.checkID(*(lastID + 0), *(lastID + 1), *(lastID + 2));
          D_LOG("id:%02X,%02X,%02X chk: %d", *(lastID + 0), *(lastID + 1), *(lastID + 2), chk);
          if (debugLevel >= 2)
          {
            if (chk || debugLevel == 3)
            {
              RFDebug(&packet, cmd);
            }
          }

          if (cmd != IthoUnknown)
          { // only act on good cmd
            if (debugLevel == 1)
            {
              RFDebug(&packet, cmd);
            }
            if (cmd == IthoLeave && remotes.remoteLearnLeaveStatus())
            {
              D_LOG("Leave command received. Trying to remove remote...");
              int result = remotes.removeRemote(*(lastID + 0), *(lastID + 1), *(lastID + 2));
              switch (result)
              {
              case -1: // failed! - remote not registered
                break;
              case -2: // failed! - no remotes registered
                break;
              case 1: // success!
                saveRemotesflag = true;
                break;
              }
            }
            if (cmd == IthoJoin)
            {
              D_LOG("Join command received. Trying to join remote...");
              D_LOG("ID:%02X,%02X,%02X", *(lastID + 0), *(lastID + 1), *(lastID + 2));
              if (remotes.remoteLearnLeaveStatus())
              {
                D_LOG("Join allowed...");

                int result = remotes.registerNewRemote(*(lastID + 0), *(lastID + 1), *(lastID + 2), remtype);
                if (result >= 0)
                {
                  D_LOG("registerNewRemote success, index:%d", result);
                  if (remotes.getRemoteFunction(result) == RemoteFunctions::BIDIRECT)
                  {
                    delay(500);
                    rf.setSendTries(1);
                    rf.sendJoinReply(*(lastID + 0), *(lastID + 1), *(lastID + 2));
                    D_LOG("Join reply send");
                    delay(100);
                    rf.send10E0();
                    D_LOG("10E0 send");
                    rf.setSendTries(3);
                  }

                  saveRemotesflag = true;
                }
                else
                {
                  D_LOG("registerNewRemote failed, code:%d", result);
                  // error
                  // case -1: // failed! - remote already registered
                  //   break;
                  // case -2: // failed! - max number of remotes reached"
                }
              }
              if (virtualRemotes.remoteLearnLeaveStatus())
              {
                int result = virtualRemotes.registerNewRemote(*(lastID + 0), *(lastID + 1), *(lastID + 2), remtype);
                if (result >= 0)
                {
                  saveVremotesflag = true;
                  delay(200);
                  toggleRemoteLLmode("vremote");
                  break;
                }
                else
                {
                  // error
                  // case -1: // failed! - remote already registered
                  //   break;
                  // case -2: // failed! - max number of remotes reached"
                }
              }
            }
            if (chk)
            {
              remotes.lastRemoteName = remotes.getRemoteNamebyIndex(remotes.remoteIndex(*(lastID + 0), *(lastID + 1), *(lastID + 2)));
              if (remotes.getRemoteFunction(remotes.remoteIndex(*(lastID + 0), *(lastID + 1), *(lastID + 2))) != RemoteFunctions::MONITOR)
              {
                if (cmd == IthoLow)
                {
                  ithoExecCommand("low", REMOTE);
                }
                if (cmd == IthoMedium)
                {
                  ithoExecCommand("medium", REMOTE);
                }
                if (cmd == IthoHigh)
                {
                  ithoExecCommand("high", REMOTE);
                }
                if (cmd == IthoTimer1)
                {
                  ithoExecCommand("timer1", REMOTE);
                }
                if (cmd == IthoTimer2)
                {
                  ithoExecCommand("timer2", REMOTE);
                }
                if (cmd == IthoTimer3)
                {
                  ithoExecCommand("timer3", REMOTE);
                }
                if (cmd == IthoCook30)
                {
                  ithoExecCommand("cook30", REMOTE);
                }
                if (cmd == IthoCook60)
                {
                  ithoExecCommand("cook60", REMOTE);
                }
                if (cmd == IthoAuto)
                {
                  ithoExecCommand("auto", REMOTE);
                }
                if (cmd == IthoAutoNight)
                {
                  ithoExecCommand("autonight", REMOTE);
                }
                if (cmd == IthoJoin && !remotes.remoteLearnLeaveStatus())
                {
                }
                if (cmd == IthoLeave && !remotes.remoteLearnLeaveStatus())
                {
                  ithoQueue.clear_queue();
                }
              }
            }
            else
            {
              // Unknown remote
            }
          }
          else
          {
            //("--- RF CMD reveiced but of unknown type ---");
          }

          const ithoRFDevices &rfDevices = rf.getRFdevices();
          for (auto &item : rfDevices.device)
          {
            if (item.deviceId[0] == 0 && item.deviceId[1] == 0 && item.deviceId[2] == 0)
              continue;
            int remIndex = remotes.remoteIndex(item.deviceId[0], item.deviceId[1], item.deviceId[2]);
            if (remIndex != -1)
            {
              remotes.addCapabilities(remIndex, "timestamp", item.timestamp);
              remotes.addCapabilities(remIndex, "lastcmd", item.lastCommand);
              if (item.co2 != 0xEFFF)
              {
                remotes.addCapabilities(remIndex, "co2", item.co2);
              }
              if (item.temp != 0xEFFF)
              {
                remotes.addCapabilities(remIndex, "temp", item.temp);
              }
              if (item.hum != 0xEFFF)
              {
                remotes.addCapabilities(remIndex, "hum", item.hum);
              }
              if (item.dewpoint != 0xEFFF)
              {
                remotes.addCapabilities(remIndex, "dewpoint", item.dewpoint);
              }
              if (item.battery != 0xEFFF)
              {
                remotes.addCapabilities(remIndex, "battery", item.battery);
              }
              if (item.pir != 0xEF)
              {
                remotes.addCapabilities(remIndex, "pir", item.pir);
              }
            }
          }
        }
      }
      TaskCC1101HWmark = uxTaskGetStackHighWaterMark(NULL);
      vTaskDelay(25 / portTICK_PERIOD_MS);
    }
  } // if (systemConfig.itho_rf_support)
  // else delete task
  TaskHandle_t xTempTask = xTaskCC1101Handle;
  xTaskCC1101Handle = NULL;
  vTaskDelete(xTempTask);
}
