
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  uint16_t val = 0;
  char s_payload[length];
  memcpy(s_payload, payload, length);
  s_payload[length] = '\0';

  //String s_topic = String(topic);
  //String s_payload = String(c_payload);


  bool updateval = true;
  if (strcmp(systemConfig.mqtt_domoticz_active, "on") == 0) {
    updateval = false;
    DynamicJsonDocument root(512);
    DeserializationError error = deserializeJson(root, s_payload);
    if (!error) {
      if (!(const char*)root[F("idx")].isNull()) {
        uint16_t idx = root[F("idx")].as<uint16_t>();
        if (idx == systemConfig.mqtt_idx) {
          if (!(const char*)root[F("svalue1")].isNull()) {
            uint16_t invalue = root[F("svalue1")].as<uint16_t>();
            double value = invalue * 2.54;
            val = (uint16_t)value;
            updateval = true;
          }
        }
      }
    }
  }
  else {
    if (strcmp(s_payload, "low") == 0) {
      val = systemConfig.itho_low;
    }
    else if (strcmp(s_payload, "medium") == 0) {
      val = systemConfig.itho_medium;
    }
    else if (strcmp(s_payload, "high") == 0) {
      val = systemConfig.itho_high;
    }
    else {
      val = strtoul (s_payload, NULL, 10);
    }
  }



  if (updateval && (strcmp(topic, systemConfig.mqtt_cmd_topic) == 0)) {

    if (val != itho_current_val) {
      writeIthoVal(val);
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
