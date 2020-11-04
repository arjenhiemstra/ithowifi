
void loop() {
  delay(1);
  yield();

  loopstart = millis();

  if (ITHOisr) {
    if (loopstart - checkin10ms > 10) {
      noInterrupts();
      ITHOisr = false;
      ITHOcheck();
      interrupts();
    }
  }

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
    if (now - lastMQTTReconnectAttempt > 5000) {

      lastMQTTReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        lastMQTTReconnectAttempt = 0;
      }
    }
  }

  if ((wifi_station_get_connect_status() != STATION_GOT_IP) && !wifiModeAP) {
    long now = millis();
    if (now - lastWIFIReconnectAttempt > 10000) {
      logInput("Attempt to reconnect WiFi");
      lastWIFIReconnectAttempt = now;
      // Attempt to reconnect
      if (connectWiFiSTA()) {
        logInput("Reconnect WiFi succesfull");
        lastWIFIReconnectAttempt = 0;
      }
      else {
        logInput("Reconnect WiFi failed!");
      }
    }
  }


  if (shouldReboot) {
    logInput("Reboot requested");
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


    if (loopstart - wifiLedUpdate >= 500) {
      wifiLedUpdate = loopstart;
      if (digitalRead(WIFILED) == LOW) {
        digitalWrite(WIFILED, HIGH);
      }
      else {
        digitalWrite(WIFILED, LOW);
      }
    }

  }
  if (loopstart - previousUpdate >= 5000 || sysStatReq) {
    Serial.print("Free heap: ");
    Serial.println(ESP.getFreeHeap());

    if (digitalRead(STATUSPIN) == LOW) {
      strcpy(i2cstat, "initok");
    }
    else {
      strcpy(i2cstat, "nok");
    }
    if (loopstart - lastSysMessage >= 1000) { //rate limit messages to once a second
      lastSysMessage = loopstart;
      sysStatReq = false;
      previousUpdate = loopstart;
      sys.updateFreeMem();
      jsonSystemstat();
    }
  }

  if (updateItho) {
    updateItho = false;
    writeIthoVal(itho_new_val);
  }

  if (loopstart - lastLog > LOGGING_INTERVAL)
  {
    sprintf(logBuff, "Mem high: %d, Mem low: %d, MQTT: %d, ITHO: %s, ITHO val: %d", sys.getMemHigh(), sys.getMemLow(), MQTT_conn_state, i2cstat, itho_current_val);
    logInput(logBuff);
    strcpy(logBuff, "");

    lastLog = loopstart;
  }

  if (ITHOhasPacket) {
    showPacket();
  }

}
