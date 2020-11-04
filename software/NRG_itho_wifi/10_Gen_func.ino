
void GetFreeMem() {
  newMem = ESP.getFreeHeap();
  if (newMem > memHigh) {
    sysStatReq = true;
    memHigh = newMem;
  }
  if (newMem < memLow) {
    sysStatReq = true;
    memLow = newMem;
  }
}


void mqttCallback(char* topic, byte* payload, unsigned int length) {

  char c_payload[length];
  memcpy(c_payload, payload, length);
  c_payload[length] = '\0';

  String s_topic = String(topic);
  String s_payload = String(c_payload);

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
      DynamicJsonDocument root(512);
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
