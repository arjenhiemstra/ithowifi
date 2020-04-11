
void loop() {
  delay(1);
  yield();
  
  loopstart = millis();
  ArduinoOTA.handle();
  ws.cleanupClients();

  if (mqttSetup) {
    mqttSetup = false;
    setupMQTTClient();
  }
  
  // handle MQTT:
  if (strcmp(systemConfig.mqtt_active, "off") == 0) {
    MQTT_conn_state_new = -5;
  }
  else {
    MQTT_conn_state_new = mqttClient.state();
  }
  if (MQTT_conn_state != MQTT_conn_state_new) {
    MQTT_conn_state = MQTT_conn_state_new;
    sysStatReq = true;
  }

  
  if (mqttClient.connected()) {
    mqttClient.loop();
  }
  else {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } 

  if (shouldReboot) {
    //Serial.println("Rebooting...");
    delay(1000);
    ESP.restart();
    delay(2000);
  }

  if (runscan) {
    sendScanDataWs();
    runscan = false;
  }
  if (wifiModeAP) {
    dnsServer.processNextRequest();
  }

 
  if(wifiModeAP) {
    if (loopstart - wifiLedUpdate >= 500) {
      wifiLedUpdate = loopstart;
      if(digitalRead(WIFILED) == LOW) {
        digitalWrite(WIFILED, HIGH);
      }
      else {
        digitalWrite(WIFILED, LOW);
      }
    }  

  }    
  if (loopstart - previousUpdate >= 5000 || sysStatReq) {
    if (loopstart - lastSysMessage >= 1000) { //rate limit messages to once a second
      lastSysMessage = loopstart;
      sysStatReq = false;
      previousUpdate = loopstart;
      GetFreeMem();
      jsonSystemstat();
    }
  }
  
  if(updateItho) {
    updateItho = false;
    writeIthoVal(itho_new_val);
  }
}
