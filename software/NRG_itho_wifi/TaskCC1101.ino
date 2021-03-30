#if defined (__HW_VERSION_TWO__)


IthoCommand RFTcommand[3] = {IthoUnknown, IthoUnknown, IthoUnknown};
byte RFTRSSI[3] = {0, 0, 0};
byte RFTcommandpos = 0;
bool RFTidChk[3] = {false, false, false};


ICACHE_RAM_ATTR void ITHOinterrupt() {
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

void RFDebug(bool chk, int * id, IthoCommand cmd) {

  strcpy(debugLog, "");
  sprintf(debugLog, "RemoteID=%d,%d,%d / Command=", id[0], id[1], id[2]);
  //log command
  switch (cmd) {
    case IthoUnknown:
      strcat(debugLog, "unknown");
      break;
    case IthoLow:
      strcat(debugLog, "low");
      break;
    case IthoMedium:
      strcat(debugLog, "medium");
      break;
    case IthoHigh:
      strcat(debugLog, "high");
      break;
    case IthoTimer1:
      strcat(debugLog, "timer1");
      break;
    case IthoTimer2:
      strcat(debugLog, "timer2");
      break;
    case IthoTimer3:
      strcat(debugLog, "timer3");
      break;
    case IthoJoin:
      strcat(debugLog, "join");
      break;
    case IthoLeave:
      strcat(debugLog, "leave");
      break;
  }
  if (chk) {
    strcat(debugLog, "<br>");
    strncat(debugLog, rf.LastMessageDecoded().c_str(), sizeof(debugLog) - strlen(debugLog) - 1);
  }
  debugLogInput = true;

}

void toggleRemoteLLmode() {
  if (remotes.toggleLearnLeaveMode()) {
    timerLearnLeaveMode.attach(1, setllModeTimer);
  }
  else {
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
      esp_task_wdt_init(1, true);
      esp_task_wdt_add(NULL);
      while (true);
    });

    //init the RF module
    rf.init();
    pinMode(ITHO_IRQ_PIN, INPUT);
    attachInterrupt(ITHO_IRQ_PIN, ITHOinterrupt, FALLING);

    //this portion of code will not be reached when no RF module is present: detach reboot script, switch on rf_supprt and load remotes config
    esp_task_wdt_add(NULL);
    reboot.detach();
    logInput("Setup: init of CC1101 RF module successful");
    rf.setDeviceID(getMac(6-3),getMac(6-2),getMac(6-1));
    
    systemConfig.itho_rf_support = 1;
    loadRemotesConfig();
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
              RFDebug(true, id, cmd);
            }
          }
          if (cmd != IthoUnknown) {  // only act on good cmd
            if (debugLevel == 1) {
              RFDebug(false, id, cmd);
            }
            if (cmd == IthoLeave && remotes.remoteLearnLeaveStatus()) {
              //Serial.print("Leave command received. Trying to remove remote... ");
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
            if (cmd == IthoJoin && remotes.remoteLearnLeaveStatus()) {
              int result = remotes.registerNewRemote(id);
              switch (result) {
                case -1: // failed! - remote already registered
                  break;
                case -2: //failed! - max number of remotes reached"
                  break;
                case 1:
                  saveRemotesflag = true;
                  break;
              }
            }
            if (chk) {
              if (cmd == IthoLow) {
                ithoSetSpeed(systemConfig.itho_low);
                ithoSetTimer((uint16_t)0);
              }
              if (cmd == IthoMedium) {
                ithoSetSpeed(systemConfig.itho_medium);
                ithoSetTimer((uint16_t)0);
              }
              if (cmd == IthoHigh) {
                ithoSetSpeed(systemConfig.itho_high);
                ithoSetTimer((uint16_t)0);
              }
              if (cmd == IthoTimer1) {
                ithoSetSpeed(systemConfig.itho_high);
                ithoSetTimer((uint16_t)systemConfig.itho_timer1);
              }
              if (cmd == IthoTimer2) {
                ithoSetSpeed(systemConfig.itho_high);
                ithoSetTimer((uint16_t)systemConfig.itho_timer2);
              }
              if (cmd == IthoTimer3) {
                ithoSetSpeed(systemConfig.itho_high);
                ithoSetTimer((uint16_t)systemConfig.itho_timer3);
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
            //Serial.print("Number of know remotes: ");
            //Serial.println(remotes.getRemoteCount());
          }
          else {
            //("--- RF CMD reveiced but of unknown type ---");
          }
        }
      }
      TaskCC1101HWmark = uxTaskGetStackHighWaterMark( NULL );
      vTaskDelay(25 / portTICK_PERIOD_MS);
    }
  } //if (systemConfig.itho_rf_support)
  //else delete task
  vTaskDelete( NULL );
}

#endif
