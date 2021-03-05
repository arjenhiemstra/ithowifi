
void mqttCallback(char* topic, byte* payload, unsigned int length) {


  if (topic == NULL) return;
  if (payload == NULL) return;

  int16_t val = -1;
  unsigned long timer = 0;
  bool dtype = true;
  if (strcmp(systemConfig.mqtt_domoticz_active, "on") == 0) {
    dtype = false;
  }

  if (length > 1023) length = 1023;

  char s_payload[length];
  memcpy(s_payload, payload, length);
  s_payload[length] = '\0';

  if (strcmp(topic, systemConfig.mqtt_cmd_topic) == 0) {
    StaticJsonDocument<1024> root;
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
           standard true, unless mqtt_domoticz_active == "on"
           if mqtt_domoticz_active == "on"
              this should be set to true first by a JSON containing key:value pair "dtype":"ithofan",
              otherwise different commands might get processed due to domoticz general domoticz/out topic structure
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

  systemConfig.itho_fallback = newState;

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

#if defined(ENABLE_SHT30_SENSOR_SUPPORT)
void updateSensor() {

  if (SHT3x_original || SHT3x_alternative) {
    if (SHT3x_original) {
      if (sht_org.readSample()) {
        Wire.endTransmission(true);
        ithoHum = sht_org.getHumidity();
        ithoTemp = sht_org.getTemperature();
        SHT3xupdated = true;
      }
    }
    if (SHT3x_alternative) {
      if (sht_alt.readSample()) {
        Wire.endTransmission(true);
        ithoHum = sht_alt.getHumidity();
        ithoTemp = sht_alt.getTemperature();
        SHT3xupdated = true;
      }
    }
  }

}
#endif

void add2queue() {
  ithoQueue.add2queue(nextIthoVal, nextIthoTimer, systemConfig.nonQ_cmd_clearsQ);
}

// Update itho Value
static void writeIthoVal(uint16_t value) {

  if (value > 254) {
    value = 254;
  }

  if (ithoCurrentVal != value) {
    uint16_t valTemp = ithoCurrentVal;
    ithoCurrentVal = value;
    sysStatReq = true;

    int timeout = 0;
    while (digitalRead(SCLPIN) == LOW && timeout < 1000) { //'check' if other master is active
      yield();
      delay(1);
      timeout++;
    }
    if (timeout != 1000) {
      updateIthoMQTT = true;

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
    }
    else {
      logInput("Warning: I2C timeout");
      ithoCurrentVal = valTemp;
      ithoQueue.add2queue(valTemp, 0, systemConfig.nonQ_cmd_clearsQ);
    }

  }

}

void printTimestamp(Print* _logOutput) {
#if defined (__HW_VERSION_ONE__)
  if (time(nullptr)) {
    time_t now;
    struct tm * timeinfo;
    time(&now);
    timeinfo = localtime(&now);

    char timeStringBuff[50];  // 50 chars should be enough
    strftime(timeStringBuff, sizeof(timeStringBuff), "%F %T ", timeinfo);
    _logOutput->print(timeStringBuff);
  } else
#elif defined (__HW_VERSION_TWO__)
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 0)) {
    char timeStringBuff[50];  // 50 chars should be enough
    strftime(timeStringBuff, sizeof(timeStringBuff), "%F %T ", &timeinfo);
    _logOutput->print(timeStringBuff);
  } else
#endif
  {
    char c[32];
    sprintf(c, "%10lu ", millis());
    _logOutput->print(c);
  }
}

void printNewline(Print* _logOutput) {
  _logOutput->print("\n");
}

void logInput(const char * inputString) {
#if defined (__HW_VERSION_TWO__)
  yield();
  if (xSemaphoreTake(mutexLogTask, (TickType_t) 500 / portTICK_PERIOD_MS) == pdTRUE) {
#endif

#if defined (ENABLE_SERIAL)
    Serial.println(inputString);
    Serial.flush();
#endif

    filePrint.open();

    Log.notice(inputString);

    filePrint.close();

#if defined (__HW_VERSION_TWO__)
    xSemaphoreGive(mutexLogTask);
  }
#endif

}
uint8_t cmdCounter = 0;


void sendButton(uint8_t number) {

  //82 60 C1 01 01 11 00 00 00 00 (4b: commander ID) (1b: command seq nr) 22 F1 03 00 02 04 (2b: seq) (1b: checksum)
  uint8_t command[] = { 0x82, 0x60, 0xC1, 0x01, 0x01, 0x11, 0x00, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x00, 0x22, 0xF1, 0x03, 0x00, 0x01, 0x04, 0x00, 0x00, 0x00 };
  
  uint8_t ID0 = getMac(3);
  uint8_t ID1 = getMac(4);
  uint8_t ID2 = getMac(5);

  command[11] = ID0;
  command[12] = ID1;
  command[13] = ID2;

  command[14] = cmdCounter;
  cmdCounter++;

  command[19] += number;

  command[sizeof(command)-1] = checksum(command, sizeof(command)-1);

  while (digitalRead(SCLPIN) == LOW ) {
    yield();
    delay(1);
  }

  Wire.beginTransmission(byte(0x41));
  for (uint8_t i = 1; i < sizeof(command); i++) {
    Wire.write(command[i]);
  }
  Wire.endTransmission(true);
}

void sendJoinI2C() {

  uint8_t command[] = { 0x82, 0x60, 0xC1, 0x01, 0x01, 0x1A, 0x00, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x00, 0x1F, 0xC9, 0x0C, 0x00, 0x22, 0xF1, 0x00, 0x00, 0x00, 0x01, 0x10, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

  uint8_t ID0 = getMac(3);
  uint8_t ID1 = getMac(4);
  uint8_t ID2 = getMac(5);
  
  command[11] = ID0;
  command[12] = ID1;
  command[13] = ID2;

  command[14] = cmdCounter;
  cmdCounter++;

  command[21] = ID0;
  command[22] = ID1;
  command[23] = ID2;

  command[27] = ID0;
  command[28] = ID1;
  command[29] = ID2;

  command[sizeof(command)-1] = checksum(command, sizeof(command)-1);

  while (digitalRead(SCLPIN) == LOW ) {
    yield();
    delay(1);
  }

  Wire.beginTransmission(byte(0x41));
  for (uint8_t i = 1; i < sizeof(command); i++) {
    Wire.write(command[i]);
  }
  Wire.endTransmission(true);


}

void sendLeaveI2C() {

  uint8_t command[] = { 0x82, 0x60, 0xC1, 0x01, 0x01, 0x14, 0x00, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x00, 0x1F, 0xC9, 0x06, 0x00, 0x1F, 0xC9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

  uint8_t ID0 = getMac(3);
  uint8_t ID1 = getMac(4);
  uint8_t ID2 = getMac(5);
//
//  ID0 = 51;
//  ID1 = 102;
//  ID2 = 153;   
  
  command[11] = ID0;
  command[12] = ID1;
  command[13] = ID2;

  command[14] = cmdCounter;
  cmdCounter++;

  command[21] = ID0;
  command[22] = ID1;
  command[23] = ID2;

  command[sizeof(command)-1] = checksum(command, sizeof(command)-1);

//  for (uint8_t i = 0; i < sizeof(command); i++) {
//    Serial.print(command[i], HEX);
//    if (i < sizeof(command)-1) Serial.print(",");
//  }

  while (digitalRead(SCLPIN) == LOW ) {
    yield();
    delay(1);
  }

  Wire.beginTransmission(byte(0x41));
  for (uint8_t i = 1; i < sizeof(command); i++) {
    Wire.write(command[i]);
  }
  Wire.endTransmission(true);
  
}

void sendQueryDevicetype() {

  uint8_t command[] = { 0x82,0x80,0x90,0xE0,0x04,0x00,0x8A };

    while (digitalRead(SCLPIN) == LOW ) {
      yield();
      delay(1);
    }
  
    Wire.beginTransmission(byte(0x41));
    for (uint8_t i = 1; i < sizeof(command); i++) {
      Wire.write(command[i]);
    }
    uint8_t err = Wire.endTransmission(true);
  
    char c[32] = {};
    uint8_t i = 0;
  
    Wire.requestFrom(byte(0x41), 25);
    while (Wire.available()) {
      c[i] = Wire.read();
      i++;
      if (i > sizeof(c)) break;
    }
  
    jsonLogMessage(c, RFLOG);  
  
}

void sendQueryStatusFormat() {

  uint8_t command[] = { 0x82,0x80,0xA4,0x00,0x04,0x00,0x56 };

  while (digitalRead(SCLPIN) == LOW ) {
    yield();
    delay(1);
  }

  Wire.beginTransmission(byte(0x41));
  for (uint8_t i = 1; i < sizeof(command); i++) {
    Wire.write(command[i]);
  }
  Wire.endTransmission(true);

}

void sendQueryStatus() {

  uint8_t command[] = { 0x82,0x80,0xA4,0x01,0x04,0x00,0x55 };

  while (digitalRead(SCLPIN) == LOW ) {
    yield();
    delay(1);
  }

  Wire.beginTransmission(byte(0x41));
  for (uint8_t i = 1; i < sizeof(command); i++) {
    Wire.write(command[i]);
  }
  Wire.endTransmission(true);

}
