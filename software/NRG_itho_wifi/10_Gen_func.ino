
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  if (topic == NULL) return;
  if (payload == NULL) return;
  
  int16_t val = -1;
  unsigned long timer = 0;
  bool dtype = true;
  if (strcmp(systemConfig.mqtt_domoticz_active, "on") == 0) {
    dtype = false;
  }

  char s_payload[length];
  memcpy(s_payload, payload, length);
  s_payload[length] = '\0';

  if (strcmp(topic, systemConfig.mqtt_cmd_topic) == 0) {
    DynamicJsonDocument root(512);
    DeserializationError error = deserializeJson(root, s_payload);
    if (!error) {
      bool jsonCmd = false;
      if (!(const char*)root["idx"].isNull()) {
        jsonCmd = true;
        //printf("JSON parse -- idx match\n");
        uint16_t idx = root["idx"].as<uint16_t>();
        if (idx == systemConfig.mqtt_idx) {
          if (!(const char*)root["svalue1"].isNull()) {
            uint16_t invalue = root["svalue1"].as<uint16_t>();
            double value = invalue * 2.54;
            val = (uint16_t)value;
            timer = 0;
          }
        }
      }
      if (!(const char*)root["dtype"].isNull()) {
        const char* value = root["dtype"] | "";
        if (strcmp(value, "ithofan") == 0) {
          dtype = true;
        }
      }
      if (dtype) {
      /*
       * standard true, unless mqtt_domoticz_active == "on"
       * if mqtt_domoticz_active == "on"
       *    this should be set to true first by a JSON containing key:value pair "dtype":"ithofan", 
       *    otherwise different commands might get processed due to domoticz general domoticz/out topic structure
       */
        if (!(const char*)root["command"].isNull()) {
          jsonCmd = true;
          const char* value = root["command"] | "";
          if (strcmp(value, "low") == 0) {
            val = systemConfig.itho_low;
            timer = 0;
          }
          else if (strcmp(value, "medium") == 0) {
            val = systemConfig.itho_medium;
            timer = 0;
          }
          else if (strcmp(value, "high") == 0) {
            val = systemConfig.itho_high;
            timer = 0;
          }
          else if (strcmp(value, "timer1") == 0) {
            val = systemConfig.itho_high;
            timer = systemConfig.itho_timer1;
          }
          else if (strcmp(value, "timer2") == 0) {
            val = systemConfig.itho_high;
            timer = systemConfig.itho_timer2;
          }
          else if (strcmp(value, "timer3") == 0) {
            val = systemConfig.itho_high;
            timer = systemConfig.itho_timer3;
          }
          if (strcmp(value, "clearqueue") == 0) {
            clearQueue = true;
          }
        }
        if (!(const char*)root["speed"].isNull()) {
          jsonCmd = true;
          val = root["speed"].as<uint16_t>();
        }
        if (!(const char*)root["timer"].isNull()) {
          jsonCmd = true;
          timer = root["timer"].as<uint16_t>();
        }
        if (!(const char*)root["clearqueue"].isNull()) {
          jsonCmd = true;
          const char* value = root["clearqueue"] | "";
          if (strcmp(value, "true") == 0) {
            clearQueue = true;
          }
        }
        if (!jsonCmd) {
          val = strtoul (s_payload, NULL, 10);
          timer = 0;
        }
      }

    }
    else {
      if (strcmp(s_payload, "low") == 0) {
        val = systemConfig.itho_low;
        timer = 0;
      }
      else if (strcmp(s_payload, "medium") == 0) {
        val = systemConfig.itho_medium;
        timer = 0;
      }
      else if (strcmp(s_payload, "high") == 0) {
        val = systemConfig.itho_high;
        timer = 0;
      }
      else if (strcmp(s_payload, "timer1") == 0) {
        val = systemConfig.itho_high;
        timer = systemConfig.itho_timer1;
      }
      else if (strcmp(s_payload, "timer2") == 0) {
        val = systemConfig.itho_high;
        timer = systemConfig.itho_timer2;
      }
      else if (strcmp(s_payload, "timer3") == 0) {
        val = systemConfig.itho_high;
        timer = systemConfig.itho_timer3;
      }
      else if (strcmp(s_payload, "clearqueue") == 0) {
        ithoQueue.clear_queue();
      }
    }

    if (val != -1) {
      nextIthoVal = val;
      nextIthoTimer = timer;
      //printf("Update -- nextIthoVal:%d, nextIthoTimer:%d\n", nextIthoVal, nextIthoTimer);
      updateItho = true;
    }
  }
  else {
    //topic unknown
  }
}

void updateState(uint16_t newState) {


  if (mqttClient.connected()) {
    char buffer[512];

    if (strcmp(systemConfig.mqtt_domoticz_active, "on") == 0) {
      int nvalue = 1;
      double state = 1.0;
      if (newState > 0) {
        state  = newState / 2.54;
      }

      newState = uint16_t(state + 0.5);
      char buf[10];
      sprintf(buf, "%d", newState);

      StaticJsonDocument<512> root;
      root["command"] = "switchlight";
      root["idx"] = systemConfig.mqtt_idx;
      root["nvalue"] = nvalue;
      root["switchcmd"] = "Set Level";
      root["level"] = buf;
      serializeJson(root, buffer);
    }
    else {
      sprintf(buffer, "%d", newState);
    }
    mqttClient.publish(systemConfig.mqtt_state_topic, buffer, true);

  }
}

void add2queue() {
  ithoQueue.add2queue(nextIthoVal, nextIthoTimer, systemConfig.nonQ_cmd_clearsQ);
}

// Update itho Value
static void writeIthoVal(uint16_t value) {

  if (value > 254) {
    value = 254;
  }

  if (ithoCurrentVal != value) {
    ithoCurrentVal = value;
    sysStatReq = true;

    while (digitalRead(SCLPIN) == LOW) { //'check' if other master is active
      yield();
    }

    Wire.beginTransmission(byte(0x00));
    delay(10);
    //write start of message
    Wire.write(byte(0x60));
    Wire.write(byte(0xC0));
    Wire.write(byte(0x20));
    Wire.write(byte(0x01));
    Wire.write(byte(0x02));

    uint8_t b = (uint8_t) value;
    uint8_t h = 0 - (67 + b);

    Wire.write(b);
    Wire.write(byte(0x00));
    Wire.write(h);

    Wire.endTransmission(true);
    updateState(value);
  }

}

void printTimestamp(Print* _logOutput) {
#if defined(ESP32)
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 0)) {
    char timeStringBuff[50];  // 50 chars should be enough
    strftime(timeStringBuff, sizeof(timeStringBuff), "<br>%F %T ", &timeinfo);
    _logOutput->print(timeStringBuff);
  } else
#elif defined(ESP8266)
  if (time(nullptr)) {
    time_t now;
    struct tm * timeinfo;
    time(&now);
    timeinfo = localtime(&now);

    char timeStringBuff[50];  // 50 chars should be enough
    strftime(timeStringBuff, sizeof(timeStringBuff), "<br>%F %T ", timeinfo);
    _logOutput->print(timeStringBuff);
  } else
#endif
  {
    char c[12];
    sprintf(c, "%10lu ", millis());
    _logOutput->print(c);
  }
}

void printNewline(Print* _logOutput) {
  _logOutput->print("\n");
}


void logInput(char * inputString) {
  filePrint.open();

  Log.notice(inputString);

  filePrint.close();

}

#if ESP32
ICACHE_RAM_ATTR void ITHOinterrupt() {
  if (digitalRead(WIFILED) == 0) {
    digitalWrite(WIFILED, 1);
  }
  else {
    digitalWrite(WIFILED, 0);
  }

  ITHOcheck();

}

ICACHE_RAM_ATTR void ITHOcheck() {

  if (rf.checkForNewPacket()) {
    int *lastID = rf.getLastID();
    int id[8];
    for (uint8_t i = 0; i < 8; i++) {
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
    if (cmd != IthoUnknown) {  // only act on good cmd
      if (cmd == IthoLeave && remotes.remoteLearnLeaveStatus()) {
        //Serial.print("Leave command received. Trying to remove remote... ");
        int result = remotes.removeRemote(id);
        switch (result) {
          case -1: // failed! - remote not registered
            break;
          case -2: // failed! - no remotes registered
            break;
          case 1: // success!
            saveRemotes = true;
            sendRemotes = true;
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
            saveRemotes = true;
            sendRemotes = true;
            break;
        }
      }
      if (chk) {
        if (cmd == IthoLow) {
          nextIthoVal = systemConfig.itho_low;
          nextIthoTimer = 0;
          updateItho = true;
        }
        if (cmd == IthoMedium) {
          nextIthoVal = systemConfig.itho_medium;
          nextIthoTimer = 0;
          updateItho = true;
        }
        if (cmd == IthoHigh || cmd == IthoFull) {
          nextIthoVal = systemConfig.itho_high;
          nextIthoTimer = 0;
          updateItho = true;
        }
        if (cmd == IthoTimer1) {
          nextIthoVal = systemConfig.itho_high;
          nextIthoTimer = systemConfig.itho_timer1;
          updateItho = true;
        }
        if (cmd == IthoTimer2) {
          nextIthoVal = systemConfig.itho_high;
          nextIthoTimer = systemConfig.itho_timer2;
          updateItho = true;
        }
        if (cmd == IthoTimer3) {
          nextIthoVal = systemConfig.itho_high;
          nextIthoTimer = systemConfig.itho_timer3;
          updateItho = true;
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
#endif
