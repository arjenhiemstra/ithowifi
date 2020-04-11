
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

void receiveEvent(size_t howMany) {
  (void) howMany;
  while (3 < Wire.available()) { // loop through all but the last 3
    char c = Wire.read(); // receive byte as a character
  }
  char received[3];
  received[0] = Wire.read();
  received[1] = Wire.read();
  received[2] = Wire.read();

  if (received[0] == 0x00 && received[1] == 0x00 && received[2] == 0xBE) {
    strcpy(i2cstat, "");
    strcat(i2cstat, "iOk");
    while (digitalRead(SCLPIN) == LOW) {
      yield();
    }
    Wire.beginTransmission(byte(0x41));
    //write response to itho fan
    Wire.write(byte(0xEF));
    Wire.write(byte(0xC0));
    Wire.write(byte(0x00));
    Wire.write(byte(0x01));
    Wire.write(byte(0x06));
    Wire.write(byte(0x00));
    Wire.write(byte(0x00));
    Wire.write(byte(0x09));
    Wire.write(byte(0x00));
    Wire.write(byte(0x09));
    Wire.write(byte(0x00));
    Wire.write(byte(0xB6));
    Wire.endTransmission();
  } else if (received[0] == 0x60 && received[1] == 0x00 && received[2] == 0x4E) {
    //response OK, init phase done. start rest of code.
    strcpy(i2cstat, "");
    strcat(i2cstat, "rOk");
    
    digitalWrite(STATUSLED, HIGH);
    //disable slave mode by detaching interrupts
    detachInterrupt(SDAPIN);
    detachInterrupt(SCLPIN);

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
