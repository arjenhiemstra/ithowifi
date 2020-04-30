
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
  
  if (s_topic == systemConfig.mqtt_cmd_topic) {    
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
