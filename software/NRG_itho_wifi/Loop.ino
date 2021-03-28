

#if defined (__HW_VERSION_ONE__)
void loop() {

  yield();

  execWebTasks();
  execMQTTTasks();
  execSystemControlTasks();
  execLogAndConfigTasks();
  execAutoPilot();

}
#endif

void execWebTasks() {
  ArduinoOTA.handle();
  ws.cleanupClients();
  if (millis() - previousUpdate >= 5000 || sysStatReq) {
    if (millis() - lastSysMessage >= 1000 && !onOTA) { //rate limit messages to once a second
      sysStatReq = false;
      lastSysMessage = millis();
      //remotes.llModeTimerUpdated = false;
      previousUpdate = millis();
      sys.updateFreeMem();
      jsonSystemstat();
    }
  }
}

void execMQTTTasks() {

  if (systemConfig.mqtt_updated) {
    systemConfig.mqtt_updated = false;
    setupMQTTClient();
  }
  // handle MQTT:
  if (systemConfig.mqtt_active == 0) {
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

    if (updateIthoMQTT) {
      updateIthoMQTT = false;
      updateState(ithoCurrentVal);
    }
#if defined(ENABLE_SHT30_SENSOR_SUPPORT)
    if (SHT3xupdated) {
      SHT3xupdated = false;
      if (mqttClient.connected()) {
        char buffer[512];

        if (systemConfig.mqtt_domoticz_active) {
          int humstat = 0;          
          // Humidity Status
          if (ithoHum < 31) {
              humstat = 2;
          } 
          else if (ithoHum > 69) {
              humstat = 3;
          } 
          else if (ithoHum > 34 && ithoHum < 66 && ithoTemp > 21 && ithoTemp < 27) {
              humstat = 1;
          }
          
          char svalue[32];
          sprintf(svalue, "%1.1f;%1.1f;%d", ithoTemp, ithoHum, humstat);

          StaticJsonDocument<512> root;
          root["svalue"] = svalue;
          root["nvalue"] = 0;
          root["idx"] = systemConfig.sensor_idx;
          serializeJson(root, buffer);
          mqttClient.publish(systemConfig.mqtt_state_topic, buffer, true);
        }
        else {
          sprintf(buffer, "{\"temp\":%1.1f,\"hum\":%1.1f}", ithoTemp, ithoHum);
          mqttClient.publish(systemConfig.mqtt_sensor_topic, buffer, true);
        }
        
      }
    }
#endif
    mqttClient.loop();
  }
  else {
    if (dontReconnectMQTT) return;
    if (millis() - lastMQTTReconnectAttempt > 5000) {

      lastMQTTReconnectAttempt = millis();
      // Attempt to reconnect
      if (reconnect()) {
        lastMQTTReconnectAttempt = 0;
      }
    }
  }
}

void execSystemControlTasks() {
  if (systemConfig.itho_sendjoin > 0 && coldBoot && ithoInit == 1 && !joinSend) {
    joinSend = true;
    sendJoinI2C();
    logInput("Virtual remote join command send");
    if (systemConfig.itho_sendjoin == 1) {
      systemConfig.itho_sendjoin = 0;
      saveSystemConfig();
    }
  }
  //Itho queue
  if (clearQueue) {
    clearQueue = false;
    ithoQueue.clear_queue();
  }
  if (updateItho) {
    updateItho = false;
    if (systemConfig.itho_rf_support) {
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
  //System control tasks
  if ((WiFi.status() != WL_CONNECTED) && !wifiModeAP) {
    if (millis() - lastWIFIReconnectAttempt > 60000) {
      logInput("Attempt to reconnect WiFi");
      lastWIFIReconnectAttempt = millis();
      // Attempt to reconnect
      if (connectWiFiSTA()) {
        logInput("Reconnect WiFi successful");
        lastWIFIReconnectAttempt = 0;
      }
      else {
        logInput("Reconnect WiFi failed!");
      }
    }
  }
#if defined (__HW_VERSION_ONE__)
  if (shouldReboot) {
    logInput("Reboot requested");
    if (!dontSaveConfig) {
      saveSystemConfig();
    }
    delay(1000);
    ESP.restart();
    delay(2000);
  }
#endif
  if (runscan) {
    runscan = false;
    scan.once_ms(10, wifiScan);
  }

  if (wifiModeAP) {
    if (millis() - APmodeTimeout > 900000) { //reboot after 15 min in AP mode
      shouldReboot = true;
    }
    dnsServer.processNextRequest();

    if (millis() - wifiLedUpdate >= 500) {
      wifiLedUpdate = millis();
      if (digitalRead(WIFILED) == LOW) {
        digitalWrite(WIFILED, HIGH);
      }
      else {
        digitalWrite(WIFILED, LOW);
      }
    }

  }
#if defined(ENABLE_SHT30_SENSOR_SUPPORT)
  if (systemConfig.syssht30) {
    if (millis() - SHT3x_readout >= 5000 && (SHT3x_original || SHT3x_alternative)) {
      SHT3x_readout = millis();
      updateSensor();
    }
  }
#endif
}

void execLogAndConfigTasks() {
  //Logging and config tasks
#if defined (__HW_VERSION_TWO__)
  if (debugLogInput) {
    debugLogInput = false;
    LogMessage.once_ms(150, []() {
      jsonLogMessage(debugLog, RFLOG);
    } );
  }
#endif
  if (saveSystemConfigflag) {
    saveSystemConfigflag = false;
    if (saveSystemConfig()) {
      jsonLogMessage(F("System settings saved successful"), WEBINTERFACE);
    }
    else {
      jsonLogMessage(F("System settings save failed: Unable to write config file"), WEBINTERFACE);
    }
  }
  if (saveWifiConfigflag) {
    saveWifiConfigflag = false;
    if (saveWifiConfig()) {
      jsonLogMessage(F("Wifi settings saved successful, reboot the device"), WEBINTERFACE);
    }
    else {
      jsonLogMessage(F("Wifi settings save failed: Unable to write config file"), WEBINTERFACE);
    }
  }
#if defined (__HW_VERSION_TWO__)
  if (saveRemotesflag) {
    saveRemotesflag = false;
    DelayedSave.once_ms(150, []() {
      saveRemotesConfig();
      jsonWsSend("ithoremotes");
    } );
  }
#endif
  if (resetWifiConfigflag) {
    resetWifiConfigflag = false;
    if (resetWifiConfig()) {
      jsonLogMessage(F("Wifi settings restored, reboot the device"), WEBINTERFACE);
    }
    else {
      jsonLogMessage(F("Wifi settings restore failed, please try again"), WEBINTERFACE);
    }
  }
  if (resetSystemConfigflag) {
    resetSystemConfigflag = false;
    if (resetSystemConfig()) {
      jsonLogMessage(F("System settings restored, reboot the device"), WEBINTERFACE);
    }
    else {
      jsonLogMessage(F("System settings restore failed, please try again"), WEBINTERFACE);
    }
  }

  if (millis() - lastLog > LOGGING_INTERVAL) {
    char logBuff[LOG_BUF_SIZE] = "";
    sprintf(logBuff, "Mem free: %d, Mem low: %d, Mem block: %d", sys.getMemHigh(), sys.getMemLow(), sys.getMaxFreeBlockSize());
    logInput(logBuff);

    lastLog = millis();
  }


}

#if defined (__HW_VERSION_TWO__)
void loop() {
  yield();
  esp_task_wdt_reset();
}

#endif
