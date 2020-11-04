

void mqttCallback(char* topic, byte* payload, unsigned int length) {

  char c_payload[length];
  memcpy(c_payload, payload, length);
  c_payload[length] = '\0';

  String s_topic = String(topic);
  String s_payload = String(c_payload);

  bool updateval = true;
  if (strcmp(systemConfig.mqtt_domoticz_active, "on") == 0) {
    updateval = false;
    StaticJsonDocument<512> root;
    DeserializationError error = deserializeJson(root, s_payload);
    if (!error) {
      if (!(const char*)root[F("idx")].isNull()) {
        uint16_t idx = root[F("idx")].as<uint16_t>();
        if (idx == systemConfig.mqtt_idx) {
          if (!(const char*)root[F("svalue1")].isNull()) {
            int16_t invalue = root[F("svalue1")].as<int16_t>();
            double value = invalue * 2.54;
            s_payload = String(value);
            updateval = true;
          }
        }
      }
    }
  }



  if (updateval && s_topic == systemConfig.mqtt_cmd_topic) {

    if (s_payload.toInt() != itho_current_val) {
      writeIthoVal((uint16_t)s_payload.toInt());
    }
  }
  else {
    //topic unknown
  }
}

void updateState(int newState) {

  if (mqttClient.connected()) {
    String payload = String(newState);

    if (strcmp(systemConfig.mqtt_domoticz_active, "on") == 0) {
      int nvalue = 1;
      double state = 1.0;
      if (newState > 0) {
        state  = newState / 2.54;
      }

      newState = int(state + 0.5);
      payload = "";
      StaticJsonDocument<512> root;
      root["command"] = "switchlight";
      root["idx"] = systemConfig.mqtt_idx;
      root["nvalue"] = nvalue;
      root["switchcmd"] = "Set Level";
      root["level"] = String(newState);
      serializeJson(root, payload);
    }

    mqttClient.publish(systemConfig.mqtt_state_topic, payload.c_str(), true);
  }
}


// Update itho Value
static void writeIthoVal(uint16_t value) {

  if (value > 254) {
    value = 254;
  }

  if (itho_current_val != value) {
    itho_current_val = value;
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
    strftime(timeStringBuff, sizeof(timeStringBuff), "%F %T ", &timeinfo);
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

ICACHE_RAM_ATTR void ITHOinterrupt() {
  ITHOisr = true;
  checkin10ms = millis();
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

      char command[10];
      switch (cmd) {
        case IthoUnknown:
          strcpy(command, "unknown");
          break;
        case IthoLow:
          strcpy(command, "low");
          break;
        case IthoMedium:
          strcpy(command, "medium");
          break;
        case IthoHigh:
          strcpy(command, "high");
          break;
        case IthoFull:
          strcpy(command, "full");
          break;
        case IthoTimer1:
          strcpy(command, "timer1");
          break;
        case IthoTimer2:
          strcpy(command, "timer2");
          break;
        case IthoTimer3:
          strcpy(command, "timer3");
          break;
        case IthoJoin:
          strcpy(command, "join");
          break;
        case IthoLeave:
          strcpy(command, "leave");
          break;
      }
      if (chk) {
        //        ITHOhasPacket = true;
        //        knownRFid = true;
        //        strncpy(lastidindex, "1", 2);
        sprintf(logBuff, "Known remote with ID:  %d:%d:%d:%d:%d:%d:%d:%d, command: %s", lastID[0], lastID[1], lastID[2], lastID[3], lastID[4], lastID[5], lastID[6], lastID[7], command);
        Serial.println(logBuff);
        strcpy(logBuff, "");

        if (strcmp(command, "leave") == 0) {
          Serial.print("Leave command received. Trying to remove remote... ");
          int result = remotes.removeRemote(id);
          switch (result) {
            case -1:
              Serial.println("failed! - remote not registered");
              break;
            case -2:
              Serial.println("failed! - no remotes registered");
              break;
            case 1:
              Serial.println("success!");
              break;
          }
        }
      }
      else {
        sprintf(logBuff, "Unknown remote with ID:  %d:%d:%d:%d:%d:%d:%d:%d, command: %s", lastID[0], lastID[1], lastID[2], lastID[3], lastID[4], lastID[5], lastID[6], lastID[7], command);
        Serial.println(logBuff);
        strcpy(logBuff, "");
        if (strcmp(command, "join") == 0) {
          Serial.print("Join command received. Trying to register remote... ");
          int result = remotes.registerNewRemote(id);
          switch (result) {
            case -1:
              Serial.println("failed! - remote already registered");
              break;
            case -2:
              Serial.println("failed! - max number of remotes reached");
              break;
            case 1:
              Serial.println("success!");
              break;
          }

        }
      }
      Serial.print("Number of know remotes: ");
      Serial.println(remotes.getRemoteCount());
    }
    else {
      //Serial.println(F("--- packet reveiced but of unknown type ---"));
    }
  }
}

void showPacket() {
  ITHOhasPacket = false;
  uint8_t goodpos = findRFTlastCommand();
  if (goodpos != -1)  RFTlastCommand = RFTcommand[goodpos];
  else                RFTlastCommand = IthoUnknown;
  //show data
  Serial.print(F("RFid known: "));
  Serial.println(knownRFid);
  Serial.print(F("RFT Current Pos: "));
  Serial.print(RFTcommandpos);
  Serial.print(F(", Good Pos: "));
  Serial.println(goodpos);
  Serial.print(F("Stored 3 commands: "));
  Serial.print(RFTcommand[0]);
  Serial.print(F(" "));
  Serial.print(RFTcommand[1]);
  Serial.print(F(" "));
  Serial.print(RFTcommand[2]);
  Serial.print(F(" / Stored 3 RSSI's:     "));
  Serial.print(RFTRSSI[0]);
  Serial.print(F(" "));
  Serial.print(RFTRSSI[1]);
  Serial.print(F(" "));
  Serial.print(RFTRSSI[2]);
  Serial.print(F(" / Stored 3 ID checks: "));
  Serial.print(RFTidChk[0]);
  Serial.print(F(" "));
  Serial.print(RFTidChk[1]);
  Serial.print(F(" "));
  Serial.print(RFTidChk[2]);
  Serial.print(F(" / Last ID: "));
  Serial.print(rf.getLastIDstr());

  Serial.print(F(" / Command = "));
  //show command
  switch (RFTlastCommand) {
    case IthoUnknown:
      Serial.print("unknown\n");
      break;
    case IthoLow:
      Serial.print("low\n");
      break;
    case IthoMedium:
      Serial.print("medium\n");
      break;
    case IthoHigh:
      Serial.print("high\n");
      break;
    case IthoFull:
      Serial.print("full\n");
      break;
    case IthoTimer1:
      Serial.print("timer1\n");
      break;
    case IthoTimer2:
      Serial.print("timer2\n");
      break;
    case IthoTimer3:
      Serial.print("timer3\n");
      break;
    case IthoJoin:
      Serial.print("join\n");
      break;
    case IthoLeave:
      Serial.print("leave\n");
      break;
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

void sendJoin() {
  rf.sendCommand(IthoJoin);
  Serial.println("sending join done.");
}
void sendLeave() {
  rf.sendCommand(IthoLeave);
  Serial.println("sending leave done.");
}
void sendStandbySpeed() {
  rf.sendCommand(IthoStandby);
  CurrentState = "Standby";
  Serial.println("sending standby done.");
}

void sendLowSpeed() {
  rf.sendCommand(IthoLow);
  CurrentState = "Low";
  Serial.println("sending low done.");
}

void sendMediumSpeed() {
  rf.sendCommand(IthoMedium);
  CurrentState = "Medium";
  Serial.println("sending medium done.");
}

void sendHighSpeed() {
  rf.sendCommand(IthoHigh);
  CurrentState = "High";
  Serial.println("sending high done.");
}

void sendFullSpeed() {
  rf.sendCommand(IthoFull);
  CurrentState = "Full";
  Serial.println("sending FullSpeed done.");
}

void sendTimer1() {
  rf.sendCommand(IthoTimer1);
  CurrentState = "Timer1";
  Serial.println("sending timer1 done.");
}
void sendTimer2() {
  rf.sendCommand(IthoTimer2);
  CurrentState = "Timer2";
  Serial.println("sending timer2 done.");
}
void sendTimer3() {
  rf.sendCommand(IthoTimer3);
  CurrentState = "Timer3";
  Serial.println("sending timer3 done.");
}


uint16_t serializeRemotes(const IthoRemote &remotes, Print& dst) {
  DynamicJsonDocument doc(4000);
  // Create an object at the root
  JsonObject root = doc.to<JsonObject>(); // Fill the object
  remotes.get(root);
  // Serialize JSON to file
  return serializeJson(doc, dst) > 0;
}

bool saveFileRemotes(const char *filename, const IthoRemote &remotes) { // Open file for writing
  File file = SPIFFS.open(filename, "w");
  if (!file) {
    Serial.println(F("Failed to create remotes file"));
    return false;
  }
  // Serialize JSON to file
  bool success = serializeRemotes(remotes, file);
  if (!success) {
    Serial.println(F("Failed to serialize remotes"));
    return false;
  }
  Serial.println(F("File saved"));
  return true;
}

bool deserializeRemotes(Stream &src, IthoRemote &remotes) {

  DynamicJsonDocument doc(4000);

  // Parse the JSON object in the file
  DeserializationError err = deserializeJson(doc, src);
  doc.shrinkToFit();
  //  Serial.print("Length: ");
  //  Serial.println(measureJson(doc));

  if (err) {
    Serial.println(F("Failed to deserialize json"));
    return false;
  }

  remotes.set(doc.as<JsonObject>());
  return true;
}

bool loadFileRemotes(const char *filename, IthoRemote &remotes) { // Open file for reading
  File file = SPIFFS.open(filename, "r");
  // This may fail if the file is missing
  if (!file) {
    Serial.println(F("Failed to open config file")); return false;
  }
  // Parse the JSON object in the file
  bool success = deserializeRemotes(file, remotes);
  // This may fail if the JSON is invalid
  if (!success) {
    Serial.println(F("Failed to deserialize configuration"));
    return false;
  }
  return true;
}
