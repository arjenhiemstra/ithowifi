
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
      jsonLogMessage(debugLog, RFLOG);
    } );

  }
  if (saveRemotes) {
    saveRemotes = false;
    DelayedSave.once_ms(150, []() {
      saveRemotesConfig();
      jsonWsSend("ithoremotes");
    } );
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
    if (loopstart - lastMQTTReconnectAttempt > 5000) {

      lastMQTTReconnectAttempt = loopstart;
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
    if (loopstart - lastWIFIReconnectAttempt > 10000) {
      logInput("Attempt to reconnect WiFi");
      lastWIFIReconnectAttempt = loopstart;
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
    if (!dontSaveConfig) {
      saveSystemConfig();
    }
    delay(1000);
    ESP.restart();
    delay(2000);
  }

  if (runscan) {
    sendScanDataWs();
    runscan = false;
  }

  if (wifiModeAP) {
    if (loopstart - APmodeTimeout > 900000) { //reboot after 15 min in AP mode
      shouldReboot = true;
    }
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
  if (loopstart - SHT3x_readout >= 5000 && (SHT3x_original || SHT3x_alternative)) {
    SHT3x_readout = loopstart;
    updateSensor();
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
    sprintf(logBuff, "Mem free: %d, Mem low: %d, MQTT: %d, ITHO: %s", sys.getMemHigh(), sys.getMemLow(), MQTT_conn_state, i2cstat);
    logInput(logBuff);
    strcpy(logBuff, "");

    lastLog = loopstart;
  }



}
