#include "tasks/task_cc1101.h"

#define TASK_CC1101_PRIO 5

// globals
Ticker TaskCC1101Timeout;
TaskHandle_t xTaskCC1101Handle = nullptr;
uint32_t TaskCC1101HWmark = 0;
uint8_t debugLevel = 0;
bool send31D9 = false;
bool send31D9debug = false;
bool send31DAdebug = false;
uint8_t status31D9{0};
bool fault31D9 = false;
bool frost31D9 = false;
bool filter31D9 = false;
bool send31DA = false;
uint8_t faninfo31DA{0};
uint8_t timer31DA{0};

// locals
StaticTask_t xTaskCC1101Buffer;
StackType_t xTaskCC1101Stack[STACK_SIZE];

// Ticker LogMessage;
Ticker timerLearnLeaveMode;

volatile bool ithoCheck = false;
SemaphoreHandle_t isrSemaphore = NULL;

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

void disableRF_ISR()
{
  detachInterrupt(itho_irq_pin);
}

void enableRF_ISR()
{
  attachInterrupt(itho_irq_pin, ITHOinterrupt, RISING);
}

void RFDebug(IthoPacket *packet)
{
  char debugLog[400] = {0};
  strncat(debugLog, rf.LastMessageDecoded(packet).c_str(), sizeof(debugLog) - strlen(debugLog) - 1);
  // log command
  switch (packet->command)
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
  case Itho31D9:
    strncat(debugLog, " (cmd:itho31d9)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case Itho31DA:
    strncat(debugLog, " (cmd:itho31da)", sizeof(debugLog) - strlen(debugLog) - 1);
    break;
  case IthoDeviceInfo:
    strncat(debugLog, " (cmd:ithodeviceinfo)", sizeof(debugLog) - strlen(debugLog) - 1);
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
      // rf.setBindAllowed(true);
      timerLearnLeaveMode.attach(1, setllModeTimer);
    }
    else
    {
      // rf.setBindAllowed(false);
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

  if (!xTaskMQTTHandle)
    startTaskMQTT();

  // reading the chip version is a non-blocking way to check CC1101 connectivity.
  // if the version is greater than 0 there has been succesful communication and we can continue with the init of the chip.
  uint8_t chipVersion = rf.getChipVersion();
  if (chipVersion)
  {
    N_LOG("Setup: CC1101 RF module found, chip version: 0x%02X", chipVersion);
  }
  else
  {
    N_LOG("Setup: no CC1101 RF module found");
    systemConfig.itho_rf_support = 0;
    systemConfig.rfInitOK = false;
  }
  if (systemConfig.itho_rf_support)
  {
    isrSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(isrSemaphore);

    // init the RF module
    rf.init();
    if (systemConfig.module_rf_id[0] == 0 && systemConfig.module_rf_id[1] == 0 && systemConfig.module_rf_id[2] == 0)
    {
      systemConfig.module_rf_id[0] = sys.getMac(3);
      systemConfig.module_rf_id[1] = sys.getMac(4);
      systemConfig.module_rf_id[2] = sys.getMac(5) - 1;
      I_LOG("rfsetup: module_rf_id default 0x%02X,0x%02X,0x%02X", systemConfig.module_rf_id[0], systemConfig.module_rf_id[1], systemConfig.module_rf_id[2]);
      saveSystemConfigflag = true;
    }
    else
    {
      I_LOG("rfsetup: module_rf_id 0x%02X,0x%02X,0x%02X", systemConfig.module_rf_id[0], systemConfig.module_rf_id[1], systemConfig.module_rf_id[2]);
    }

    rf.setDefaultID(systemConfig.module_rf_id[0], systemConfig.module_rf_id[1], systemConfig.module_rf_id[2]);
    rf.setBindAllowed(false);
    rf.setAllowAll(false);

    pinMode(itho_irq_pin, INPUT);
    enableRF_ISR();

    systemConfig.itho_rf_support = 1;
    loadRemotesConfig("flash");
    for (int index = 0; index < remotes.getRemoteCount(); index++)
    {
      uint8_t id[3]{};
      remotes.getRemoteIDbyIndex(index, &id[0]);
      rf.updateRFDevice(index, id[0], id[1], id[2], remotes.getRemoteType(index), remotes.getRemoteFunction(index) == RemoteFunctions::BIDIRECT ? true : false);
    }
    systemConfig.rfInitOK = true;
    saveSystemConfigflag = true;
    bool sendJoinReply = false;
    bool send10E0 = false;

    uint8_t joinReplyRemIndex{255};
    // uint8_t remIndex10E0{255};

    for (;;)
    {
      yield();
      esp_task_wdt_reset();

      TaskCC1101Timeout.once_ms(3000, []()
                                { W_LOG("Warning: CC1101 Task timed out!"); });

      if (send10E0 && !ithoCheck)
      {
        send10E0 = false;
        rf.setSendTries(1);
        disableRF_ISR();
        rf.send10E0();
        enableRF_ISR();
        N_LOG("send10E0 send");
        rf.setSendTries(3);
      }
      if (sendJoinReply && !ithoCheck)
      {
        sendJoinReply = false;
        rf.setSendTries(1);
        disableRF_ISR();
        int res = rf.sendJoinReply(joinReplyRemIndex);
        enableRF_ISR();
        N_LOG("Join reply send, result:%d", res);
        rf.setSendTries(3);
        joinReplyRemIndex = 255;
      }
      if ((send31D9 || send31D9debug) && !ithoCheck)
      {
        N_LOG("send31D9");
        // 80 82 B1 D9 01 10 86 05 0A 20 20 20 20 20 20 20 20 20 20 20 20 00 4E
        uint8_t command[] = {0x31, 0xD9, 0x11, 0x00, 0x06, 0x00, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00};

        if (send31D9)
        {
          send31D9 = false;

          uint8_t receive_buffer[256]{};
          size_t result = sendQuery31D9(&receive_buffer[0]);

          if (result > 4)
          {
            for (uint8_t i = 4; i < sizeof(command); i++)
            {
              command[i] = receive_buffer[i + 2];
            }
          }
        }
        if (send31D9debug)
        {
          send31D9debug = false;
          uint8_t speedstatus = status31D9;
          uint8_t fanstate = 0;
          fanstate |= fault31D9 ? 0x80 : 0x00;
          fanstate |= frost31D9 ? 0x40 : 0x00;
          fanstate |= filter31D9 ? 0x20 : 0x00;
          command[5] = speedstatus;
          command[4] = fanstate;
        }
        rf.setSendTries(1);
        disableRF_ISR();
        rf.send31D9(&command[0]);
        enableRF_ISR();
        rf.setSendTries(3);
      }
      if ((send31DA || send31DAdebug) && !ithoCheck)
      {
        N_LOG("send31DA");
        uint8_t command[] = {0x31, 0xDA, 0x1D, 0x00, 0xC8, 0x40, 0xEF, 0x7F, 0xFF, 0x7F, 0xFF, 0x7F, 0xFF, 0x7F, 0xFF, 0x7F, 0xFF, 0xEF, 0xF8, 0x08, 0xEF, 0x00, 0x00, 0x00, 0x00, 0x00, 0xEF, 0xEF, 0x7F, 0xFF, 0x7F, 0xFF};

        if (send31DA)
        {
          send31DA = false;

          uint8_t receive_buffer[256]{};
          size_t result = sendQuery31DA(&receive_buffer[0]);
          // 80 82 B1 DA 01 1C C8 40 03 A5 39 EF 7F FF 7F FF 7F FF 7F FF F8 08 EF 98 05 00 00 00 EF EF 7F FF 7F FF 20
          // 1D:            00,C8,40,04,7B,38,EF,7F,FF,7F,FF,7F,FF,7F,FF,F8,08,EF,98,05,00,00,00,EF,EF,7F,FF,7F,FF

          if (result > 4)
          {
            for (uint8_t i = 4; i < sizeof(command); i++)
            {
              command[i] = receive_buffer[i + 2];
            }
          }
          else
          {
            E_LOG("sendQuery31DA failed");
          }
        }
        if (send31DAdebug)
        {
          send31DAdebug = false;

          command[21] = faninfo31DA;
          command[24] = timer31DA;
          
        }

        rf.setSendTries(1);
        disableRF_ISR();
        rf.send31DA(&command[0]);
        enableRF_ISR();
        rf.setSendTries(3);
      }
      if (ithoCheck)
      {
        ithoCheck = false;

        // D_LOG("getpacketBufferCount: %d", rf.getpacketBufferCount());

        // while (rf.getpacketBufferCount() > 0)
        // {
        IthoPacket *packet = rf.checkForNewPacket();
        rf.parseMessage(packet);

        uint8_t *lastID = rf.getLastID(packet);
        IthoCommand cmd = rf.getLastCommand(packet);
        RemoteTypes remtype = rf.getLastRemType(packet);
        bool chk = remotes.checkID(*(lastID + 0), *(lastID + 1), *(lastID + 2));
        if (debugLevel >= 2)
        {
          if (chk || debugLevel == 3)
          {
            RFDebug(packet);
          }
        }
        if (cmd != IthoUnknown)
        { // only act on good cmd
          if (debugLevel == 1)
          {
            RFDebug(packet);
          }
          if (cmd == IthoLeave && remotes.remoteLearnLeaveStatus())
          {
            D_LOG("Leave command received. Trying to remove remote...");
            int index = remotes.removeRemote(*(lastID + 0), *(lastID + 1), *(lastID + 2));
            if (index >= 0)
            {
              rf.removeRFDevice(index);
            }
            switch (index)
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
          if (cmd == IthoJoin || (cmd == IthoLow && remtype == RemoteTypes::RFTSPIDER)) // Itho low for the spider because join can be difficult to send
          {
            D_LOG("Join command received. Trying to join remote...");
            if (remotes.remoteLearnLeaveStatus())
            {
              int index = remotes.registerNewRemote(*(lastID + 0), *(lastID + 1), *(lastID + 2), remtype);
              rf.updateRFDevice(index, *(lastID + 0), *(lastID + 1), *(lastID + 2), remotes.getRemoteType(index), remotes.getRemoteFunction(index) == RemoteFunctions::BIDIRECT ? true : false);
              if (index >= 0)
              {
                // int rfresult = rf.getRemoteIndexByID(*(lastID + 0), *(lastID + 1), *(lastID + 2));
                if (remotes.getRemoteFunction(index) == RemoteFunctions::BIDIRECT)
                {
                  // rf.updateRFDestinationID(sys.getMac(3), sys.getMac(4), sys.getMac(5) - 1, index);
                  if (index >= 0)
                  {
                    joinReplyRemIndex = index;
                    sendJoinReply = true;
                  }
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
              int result = virtualRemotes.registerNewRemote(*(lastID + 0), *(lastID + 1), *(lastID + 2), remtype);
              D_LOG("registerNewRemote:%d", result);

              if (result >= 0)
              {
                saveVremotesflag = true;
                // delay(200);
                toggleRemoteLLmode("vremote");
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
            int index = remotes.remoteIndex(*(lastID + 0), *(lastID + 1), *(lastID + 2));
            remotes.lastRemoteName = remotes.getRemoteNamebyIndex(index);
            RemoteFunctions remfunc = remotes.getRemoteFunction(index);
            if (remfunc != RemoteFunctions::MONITOR)
            {
              D_LOG("remfunc:%d", remfunc);
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
              if (cmd == Itho31D9)
              {
                send31D9 = true;
              }
              if (cmd == Itho31DA)
              {
                send31DA = true;
              }
              if (cmd == IthoDeviceInfo)
              {
                send10E0 = true;
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
          if (item.remoteID[0] == 0 && item.remoteID[1] == 0 && item.remoteID[2] == 0)
            continue;
          if(item.bidirectional && (cmd != IthoJoin && cmd != IthoLeave)) { //trigger fan status update if there is a bi-directional remote in the known list of remotes
            send31D9 = true;
            send31DA = true;
          }
          int remIndex = remotes.remoteIndex(item.remoteID[0], item.remoteID[1], item.remoteID[2]);
          if (remIndex >= 0)
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
            if (item.setpoint != 0xEFFF)
            {
              remotes.addCapabilities(remIndex, "setpoint", item.setpoint);
            }
            //
          }
        }
        //} //while
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
