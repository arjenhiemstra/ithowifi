

IthoCommand RFTcommand[3] = {IthoUnknown, IthoUnknown, IthoUnknown};
byte RFTRSSI[3] = {0, 0, 0};
byte RFTcommandpos = 0;
bool RFTidChk[3] = {false, false, false};
//Ticker LogMessage;

IRAM_ATTR void ITHOinterrupt() {
  rf.receivePacket();
  ithoCheck = true;
}

void disableRFsupport() {
  detachInterrupt(ITHO_IRQ_PIN);
}


uint8_t findRFTlastCommand() {
  if (RFTcommand[RFTcommandpos] != IthoUnknown)               return RFTcommandpos;
  if ((RFTcommandpos == 0) && (RFTcommand[2] != IthoUnknown)) return 2;
  if ((RFTcommandpos == 0) && (RFTcommand[1] != IthoUnknown)) return 1;
  if ((RFTcommandpos == 1) && (RFTcommand[0] != IthoUnknown)) return 0;
  if ((RFTcommandpos == 1) && (RFTcommand[2] != IthoUnknown)) return 2;
  if ((RFTcommandpos == 2) && (RFTcommand[1] != IthoUnknown)) return 1;
  if ((RFTcommandpos == 2) && (RFTcommand[0] != IthoUnknown)) return 0;
  return -1;
}

void RFDebug(IthoCommand cmd) {
  char debugLog[200];
  strncat(debugLog, rf.LastMessageDecoded().c_str(), sizeof(debugLog) - strlen(debugLog) - 1);
  //log command
  switch (cmd) {
    case IthoUnknown:
      strncat(debugLog, " (cmd:unknown)", sizeof(debugLog) - strlen(debugLog) - 1);
      break;
    case IthoStandby:
      strncat(debugLog, " (cmd:standby)", sizeof(debugLog) - strlen(debugLog) - 1);
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
    case IthoTimer1:
      strncat(debugLog, " (cmd:timer1)", sizeof(debugLog) - strlen(debugLog) - 1);
      break;
    case IthoTimer2:
      strncat(debugLog, " (cmd:timer2)", sizeof(debugLog) - strlen(debugLog) - 1);
      break;
    case IthoTimer3:
      strncat(debugLog, " (cmd:timer3)", sizeof(debugLog) - strlen(debugLog) - 1);
      break;
    case IthoJoin:
      strncat(debugLog, " (cmd:join)", sizeof(debugLog) - strlen(debugLog) - 1);
      break;
    case IthoLeave:
      strncat(debugLog, " (cmd:leave)", sizeof(debugLog) - strlen(debugLog) - 1);
      break;

  }
  //  LogMessage.once_ms(150, []() {
  logMessagejson(debugLog, RFLOG);
  //    } );

}

void toggleRemoteLLmode() {
  if (remotes.toggleLearnLeaveMode()) {
    rf.setBindAllowed(true);
    timerLearnLeaveMode.attach(1, setllModeTimer);
  }
  else {
    rf.setBindAllowed(false);
    timerLearnLeaveMode.detach();
    remotes.setllModeTime(0);
    sysStatReq = true;
  }
}

void setllModeTimer() {
  remotes.updatellModeTimer();
  sysStatReq = true;
  if (remotes.getllModeTime() == 0) {
    timerLearnLeaveMode.detach();
  }

}

void toggleCCcal() {
  if (rf.getCCcalEnabled()) {
    abortCCcal();
    timerCCcal.detach();
    ithoCCstatReq = true;
  }
  else {
    rf.setCCcalEnable(1);
    timerCCcal.attach(1, setCCcalTimer);
  }
}

void setCCcalTimer() {
  updateCCcalData();
  if (!rf.getCCcalEnabled()) {
    timerCCcal.detach();
  }

}

void updateCCcalData() {
  uint32_t currentF = rf.getCCcal();
  uint16_t curTimeoutSetting = rf.getCCcalTimeout();
  uint32_t stepTimeout = rf.getCCcalTimer();
  uint8_t calEnabled = rf.getCCcalEnabled();
  uint8_t calFinished = rf.getCCcalFinised();

  StaticJsonDocument<500> root;

  JsonObject systemstat = root.createNestedObject("ccstatus");
  systemstat["currentF"] = currentF;
  systemstat["curTSet"] = curTimeoutSetting;
  systemstat["stepT"] = stepTimeout;
  systemstat["calEnabled"] = calEnabled;
  systemstat["calFin"] = calFinished;

  notifyClients(root.as<JsonObjectConst>());

}



void abortCCcal() {
  rf.abortCCcal();
}

void resetCCcal() {
  rf.resetCCcal();
}

void startTaskCC1101() {
  xTaskCC1101Handle = xTaskCreateStaticPinnedToCore(
                        TaskCC1101,
                        "TaskCC1101",
                        STACK_SIZE,
                        ( void * ) 1,
                        TASK_CC1101_PRIO,
                        xTaskCC1101Stack,
                        &xTaskCC1101Buffer,
                        CONFIG_ARDUINO_RUNNING_CORE);
}

void TaskCC1101( void * pvParameters ) {
  configASSERT( ( uint32_t ) pvParameters == 1UL );


  startTaskMQTT();

  if (systemConfig.itho_rf_support) {
    Ticker reboot;

    //switch off rf_support
    systemConfig.itho_rf_support = 0;
    systemConfig.rfInitOK = false;

    //attach saveConfig and reboot script to fire after 2 sec
    reboot.attach(2, []() {
      logInput("Setup: init of CC1101 RF module failed");
      saveSystemConfig();
      delay(1000);
      ACTIVE_FS.end();
      esp_task_wdt_init(1, true);
      esp_task_wdt_add(NULL);
      while (true);
    });

    //init the RF module
    rf.init();
    pinMode(ITHO_IRQ_PIN, INPUT);
    attachInterrupt(ITHO_IRQ_PIN, ITHOinterrupt, RISING);

    //this portion of code will not be reached when no RF module is present: detach reboot script, switch on rf_supprt and load remotes config
    esp_task_wdt_add(NULL);
    reboot.detach();
    logInput("Setup: init of CC1101 RF module successful");
    rf.setDeviceID(getMac(6 - 3), getMac(6 - 2), getMac(6 - 1));
    systemConfig.itho_rf_support = 1;
    loadRemotesConfig();
    rf.setBindAllowed(true);
    for (int i = 0; i < remotes.getRemoteCount(); i++) {
      const int *id = remotes.getRemoteIDbyIndex(i);
      rf.addRFDevice(*id, *(id + 1), *(id + 2));
    }
    rf.setBindAllowed(false);
    rf.setAllowAll(false);

    systemConfig.rfInitOK = true;

    for (;;) {
      yield();
      esp_task_wdt_reset();

      TaskCC1101Timeout.once_ms(1000, []() {
        logInput("Error: CC1101 Task timed out!");
      });
      if (ithoCheck) {
        ithoCheck = false;
        if (rf.checkForNewPacket()) {

          int *lastID = rf.getLastID();
          int id[3];
          for (uint8_t i = 0; i < 3; i++) {
            id[i] = lastID[i];
          }
          IthoCommand cmd = rf.getLastCommand();
          if (++RFTcommandpos > 2) RFTcommandpos = 0;  // store information in next entry of ringbuffers
          RFTcommand[RFTcommandpos] = cmd;
          RFTRSSI[RFTcommandpos]    = rf.ReadRSSI();
          //int *lastID = rf.getLastID();
          bool chk = remotes.checkID(id);
          //bool chk = rf.checkID(RFTid);
          RFTidChk[RFTcommandpos]   = chk;

          if (debugLevel >= 2) {
            if (chk || debugLevel == 3) {
              RFDebug(cmd);
            }
          }
          if (cmd != IthoUnknown) {  // only act on good cmd
            if (debugLevel == 1) {
              RFDebug(cmd);
            }
            if (cmd == IthoLeave && remotes.remoteLearnLeaveStatus()) {
              D_LOG("Leave command received. Trying to remove remote...\n");
              int result = remotes.removeRemote(id);
              switch (result) {
                case -1: // failed! - remote not registered
                  break;
                case -2: // failed! - no remotes registered
                  break;
                case 1: // success!
                  saveRemotesflag = true;
                  break;
              }
            }
            if (cmd == IthoJoin) {
              if (remotes.remoteLearnLeaveStatus()) {
                //char logBuff[LOG_BUF_SIZE] = "";
                int result = remotes.registerNewRemote(id);
                switch (result) {
                  case -1: // failed! - remote already registered
                    break;
                  case -2: //failed! - max number of remotes reached"
                    break;
                  case 1:
                    saveRemotesflag = true;
                    break;
                    //default:
                }
              }
            }
            if (chk) {
              remotes.lastRemoteName = remotes.getRemoteNamebyIndex(remotes.remoteIndex(id));
              if (cmd == IthoLow) {
                ithoExecCommand("low", REMOTE);
              }
              if (cmd == IthoMedium) {
                ithoExecCommand("medium", REMOTE);
              }
              if (cmd == IthoHigh) {
                ithoExecCommand("high", REMOTE);
              }
              if (cmd == IthoTimer1) {
                ithoExecCommand("timer1", REMOTE);
              }
              if (cmd == IthoTimer2) {
                ithoExecCommand("timer2", REMOTE);
              }
              if (cmd == IthoTimer3) {
                ithoExecCommand("timer3", REMOTE);
              }
              if (cmd == IthoJoin && !remotes.remoteLearnLeaveStatus()) {
              }
              if (cmd == IthoLeave && !remotes.remoteLearnLeaveStatus()) {
                ithoQueue.clear_queue();
              }

            }
            else {
              //Unknown remote
            }
            D_LOG("Number of know remotes: %d\n", remotes.getRemoteCount());
          }
          else {
            //("--- RF CMD reveiced but of unknown type ---");
          }


          const ithoRFDevices &rfDevices = rf.getRFdevices();
          for (auto& item : rfDevices.device) {
            if (item.deviceId == 0) continue;
            int remIndex = remotes.remoteIndex(item.deviceId);            
            if (remIndex != -1) {
              remotes.addCapabilities(remIndex, "lastcmd", item.lastCommand);
              if (item.co2 != 0xEFFF) {
                remotes.addCapabilities(remIndex, "co2", item.co2);
              }
              if (item.temp != 0xEFFF) {
                remotes.addCapabilities(remIndex, "temp", item.temp);
              }
              if (item.hum != 0xEFFF) {
                remotes.addCapabilities(remIndex, "hum", item.hum);
              }
              if (item.dewpoint != 0xEFFF) {
                remotes.addCapabilities(remIndex, "dewpoint", item.dewpoint);
              }
              if (item.battery != 0xEFFF) {
                remotes.addCapabilities(remIndex, "battery", item.battery);
              }
            }
          }

        }
      }
      if (ithoCCstatReq) {
        ithoCCstatReq = false;
        updateCCcalData();
      }
      TaskCC1101HWmark = uxTaskGetStackHighWaterMark( NULL );
      vTaskDelay(25 / portTICK_PERIOD_MS);
    }
  } //if (systemConfig.itho_rf_support)
  //else delete task
  vTaskDelete( NULL );
}
