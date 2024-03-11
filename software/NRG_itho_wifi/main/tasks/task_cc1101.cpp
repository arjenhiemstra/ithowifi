#include "tasks/task_cc1101.h"

#define TASK_CC1101_PRIO 5

// globals
Ticker TaskCC1101Timeout;
TaskHandle_t xTaskCC1101Handle = nullptr;
uint32_t TaskCC1101HWmark = 0;
uint8_t debugLevel = 0;

// locals
StaticTask_t xTaskCC1101Buffer;
StackType_t xTaskCC1101Stack[STACK_SIZE];

IthoCommand RFTcommand[3] = {IthoUnknown, IthoUnknown, IthoUnknown};
byte RFTRSSI[3] = {0, 0, 0};
byte RFTcommandpos = 0;
bool RFTidChk[3] = {false, false, false};
// Ticker LogMessage;
Ticker timerLearnLeaveMode;

volatile bool ithoCheck = false;

IRAM_ATTR void ITHOinterrupt()
{
  ithoCheck = true;
}

void disableRFsupport()
{
  detachInterrupt(itho_irq_pin);
}

uint8_t findRFTlastCommand()
{
  if (RFTcommand[RFTcommandpos] != IthoUnknown)
    return RFTcommandpos;
  if ((RFTcommandpos == 0) && (RFTcommand[2] != IthoUnknown))
    return 2;
  if ((RFTcommandpos == 0) && (RFTcommand[1] != IthoUnknown))
    return 1;
  if ((RFTcommandpos == 1) && (RFTcommand[0] != IthoUnknown))
    return 0;
  if ((RFTcommandpos == 1) && (RFTcommand[2] != IthoUnknown))
    return 2;
  if ((RFTcommandpos == 2) && (RFTcommand[1] != IthoUnknown))
    return 1;
  if ((RFTcommandpos == 2) && (RFTcommand[0] != IthoUnknown))
    return 0;
  return -1;
}

void RFDebug(IthoCommand cmd)
{
  char debugLog[400] = {0};
  strncat(debugLog, rf.LastMessageDecoded().c_str(), sizeof(debugLog) - strlen(debugLog) - 1);
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
      STACK_SIZE,
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

    // switch off rf_support
    systemConfig.itho_rf_support = 0;
    systemConfig.rfInitOK = false;

    // attach saveConfig and reboot script to fire after 2 sec
    reboot.attach(2, []()
                  {
      E_LOG("Setup: init of CC1101 RF module failed");
      saveSystemConfig("flash");
      delay(1000);
      ACTIVE_FS.end();
      esp_restart(); });

    // init the RF module
    rf.init();
    pinMode(itho_irq_pin, INPUT);
    attachInterrupt(itho_irq_pin, ITHOinterrupt, RISING);

    // this portion of code will not be reached when no RF module is present: detach reboot script, switch on rf_supprt and load remotes config
    esp_task_wdt_add(NULL);
    reboot.detach();
    N_LOG("Setup: init of CC1101 RF module successful");
    rf.setDeviceIDsend(sys.getMac(3), sys.getMac(4), sys.getMac(5) - 1);
    systemConfig.itho_rf_support = 1;
    loadRemotesConfig("flash");
    // rf.setBindAllowed(true);
    for (int i = 0; i < remotes.getRemoteCount(); i++)
    {
      const int *id = remotes.getRemoteIDbyIndex(i);
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
        if (rf.checkForNewPacket())
        {
          int *lastID = rf.getLastID();
          int id[3];
          for (uint8_t i = 0; i < 3; i++)
          {
            id[i] = *(&lastID[0] + i);
          }
          IthoCommand cmd = rf.getLastCommand();
          RemoteTypes remtype = rf.getLastRemType();
          if (++RFTcommandpos > 2)
            RFTcommandpos = 0; // store information in next entry of ringbuffers
          RFTcommand[RFTcommandpos] = cmd;
          RFTRSSI[RFTcommandpos] = rf.ReadRSSI();
          // int *lastID = rf.getLastID();
          bool chk = remotes.checkID(id);
          // bool chk = rf.checkID(RFTid);
          RFTidChk[RFTcommandpos] = chk;
          if (debugLevel >= 2)
          {
            if (chk || debugLevel == 3)
            {
              RFDebug(cmd);
            }
          }
          if (cmd != IthoUnknown)
          { // only act on good cmd
            if (debugLevel == 1)
            {
              RFDebug(cmd);
            }
            if (cmd == IthoLeave && remotes.remoteLearnLeaveStatus())
            {
              D_LOG("Leave command received. Trying to remove remote...");
              int result = remotes.removeRemote(id);
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
              D_LOG("ID:%02X,%02X,%02X", id[0], id[1], id[2]);
              if (remotes.remoteLearnLeaveStatus())
              {
                int result = remotes.registerNewRemote(id, remtype);
                if (result >= 0)
                {
                  if (remotes.getRemoteFunction(result) == RemoteFunctions::BIDIRECT)
                  {
                    // delay(500);
                    // rf.setSendTries(1);
                    // rf.sendJoinReply(id[0],id[1],id[2]);
                    // D_LOG("Join reply send");
                    // delay(100);
                    // rf.send10E0();
                    // D_LOG("10E0 send");
                    // rf.setSendTries(3);
                  }

                  saveRemotesflag = true;
                }
                else
                {
                  // error
                  // case -1: // failed! - remote already registered
                  //   break;
                  // case -2: // failed! - max number of remotes reached"
                }
              }
              if (virtualRemotes.remoteLearnLeaveStatus())
              {
                int result = virtualRemotes.registerNewRemote(id, remtype);
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
              remotes.lastRemoteName = remotes.getRemoteNamebyIndex(remotes.remoteIndex(id));
              if (remotes.getRemoteFunction(remotes.remoteIndex(id)) != RemoteFunctions::MONITOR)
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
            if (item.deviceId == 0)
              continue;
            int remIndex = remotes.remoteIndex(item.deviceId);
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
