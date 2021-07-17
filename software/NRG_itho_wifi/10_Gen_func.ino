
void mqttCallback(char* topic, byte* payload, unsigned int length) {


  if (topic == NULL) return;
  if (payload == NULL) return;

  int16_t val = -1;
  unsigned long timer = 0;
  bool dtype = true;
  if (systemConfig.mqtt_domoticz_active) {
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

    if (systemConfig.mqtt_domoticz_active) {
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
bool writeIthoVal(uint16_t value) {

  if (value > 254) {
    value = 254;
  }

  if (ithoCurrentVal != value) {
    uint16_t valTemp = ithoCurrentVal;
    ithoCurrentVal = value;

    int timeout = 0;
    while (digitalRead(SCLPIN) == LOW && timeout < 1000) { //'check' if other master is active
      yield();
      delay(1);
      timeout++;
    }
    if (timeout != 1000) {

      if (systemConfig.itho_forcemedium) {
        sendButton(2);
        delay(25);
      }
      updateIthoMQTT = true;

      uint8_t command[] = {0x00, 0x60, 0xC0, 0x20, 0x01, 0x02, 0xFF, 0x00, 0xFF};

      uint8_t b = (uint8_t) value;

      command[6] = b;
      //command[8] = 0 - (67 + b);
      command[sizeof(command) - 1] = checksum(command, sizeof(command) - 1);

#if defined (__HW_VERSION_ONE__)
      Wire.beginTransmission(byte(0x00));
      for (uint8_t i = 1; i < sizeof(command); i++) {
        Wire.write(command[i]);
      }
      Wire.endTransmission(true);
#elif defined (__HW_VERSION_TWO__)
      i2c_sendBytes(command, sizeof(command));
#endif

    }
    else {
      logInput("Warning: I2C timeout");
      ithoCurrentVal = valTemp;
      ithoQueue.add2queue(valTemp, 0, systemConfig.nonQ_cmd_clearsQ);
    }
    return true;
  }
  return false;
}

void receiveEvent(size_t howMany) {

  char localbuf[512] = "";
  uint16_t pos = 0;

  while (Wire.available()) { // loop through all but the last 3
    localbuf[pos] = Wire.read(); // receive byte as a character
    //if(pos > sizeof(localbuf)) break;
    pos++;
  }

  std::string s;
  s.reserve(pos * 3 + 2);
  for (size_t i = 0; i < pos; ++i) {
    if (i)
      s += ' ';
    s += toHex(localbuf[i] >> 4);
    s += toHex(localbuf[i] & 0xF);
  }
  strcpy(i2c_slave_buf, "");
  strlcpy(i2c_slave_buf, s.c_str(), sizeof(i2c_slave_buf));

  callback_called = true;
}

void printTimestamp(Print * _logOutput) {
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

void printNewline(Print * _logOutput) {
  _logOutput->print("\n");
}

void logInput(const char * inputString) {
#if defined (__HW_VERSION_TWO__)
  yield();
  if (xSemaphoreTake(mutexLogTask, (TickType_t) 500 / portTICK_PERIOD_MS) == pdTRUE) {
#endif

    filePrint.open();

    Log.notice(inputString);

    filePrint.close();

#if defined (__HW_VERSION_TWO__)
    xSemaphoreGive(mutexLogTask);
  }
#endif


}
