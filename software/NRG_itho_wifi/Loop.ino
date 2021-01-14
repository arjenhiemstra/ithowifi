
void loop() {
  delay(1);
  yield();

  loopstart = millis();
  ArduinoOTA.handle();
  ws.cleanupClients();

  if (systemConfig.mqtt_updated) {
    systemConfig.mqtt_updated = false;
    setupMQTTClient();
  }
  if (clearQueue) {
    clearQueue = false;
    ithoQueue.clear_queue();
  }
#if defined (__HW_VERSION_TWO__)
  if (debugLogInput) {

    debugLogInput = false;
    LogMessage.once_ms(150, []() {

      jsonMessageBox(debugLog, debugLogMsg2);
      //logInput(debugLog);
    } );

  }
  if (ithoCheck) {
    ithoCheck = false;
    ITHOcheck();
  }
  if (saveRemotes) {
    saveRemotes = false;
    saveRemotesConfig();
  }
  if (sendRemotes) {
    sendRemotes = false;
    jsonWsSend("ithoremotes");
  }
#endif
  if (updateItho) {
    updateItho = false;
    if (strcmp(systemConfig.itho_rf_support, "on") == 0) {
      IthoCMD.once_ms(150, add2queue);
    }
    else {
      ithoQueue.add2queue(nextIthoVal, nextIthoTimer, systemConfig.nonQ_cmd_clearsQ);
    }
  }
  if (ithoQueue.ithoSpeedUpdated) {
    ithoQueue.ithoSpeedUpdated = false;
    writeIthoVal(ithoQueue.get_itho_speed());
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
#if defined (__HW_VERSION_ONE__)
  if ((wifi_station_get_connect_status() != STATION_GOT_IP) && !wifiModeAP) {
#elif defined (__HW_VERSION_TWO__)
  if ((WiFi.status() != WL_CONNECTED) && !wifiModeAP) {
#endif
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

    if (digitalRead(STATUSPIN) == LOW) {
      strcpy(i2cstat, "initok");
    }
    else {
      strcpy(i2cstat, "nok");
    }
    if (loopstart - lastSysMessage >= 1000) { //rate limit messages to once a second
      lastSysMessage = loopstart;
      //remotes.llModeTimerUpdated = false;
      sysStatReq = false;
      previousUpdate = loopstart;
      sys.updateFreeMem();
      jsonSystemstat();
    }
  }

  if (loopstart - lastLog > LOGGING_INTERVAL) {
    sprintf(logBuff, "Mem high: %d, Mem low: %d, MQTT: %d, ITHO: %s, ITHO val: %d", sys.getMemHigh(), sys.getMemLow(), MQTT_conn_state, i2cstat, ithoCurrentVal);
    logInput(logBuff);
    strcpy(logBuff, "");

    lastLog = loopstart;
  }



}
