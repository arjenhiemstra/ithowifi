#if defined (HW_VERSION_TWO)

void startTaskSysControl() {
  xTaskSysControlHandle = xTaskCreateStaticPinnedToCore(
                            TaskSysControl,
                            "TaskSysControl",
                            STACK_SIZE_LARGE,
                            ( void * ) 1,
                            TASK_SYS_CONTROL_PRIO,
                            xTaskSysControlStack,
                            &xTaskSysControlBuffer,
                            CONFIG_ARDUINO_RUNNING_CORE);
}

void TaskSysControl( void * pvParameters ) {
  configASSERT( ( uint32_t ) pvParameters == 1UL );
  Ticker TaskTimeout;

  wifiInit();

  init_vRemote();

#if defined(ENABLE_SHT30_SENSOR_SUPPORT)
  initSensor();
#endif

  startTaskCC1101();

  esp_task_wdt_add(NULL);

  for (;;) {
    yield();
    esp_task_wdt_reset();

    TaskTimeout.once_ms(1000, []() {
      logInput("Error: Task SysControl timed out!");
    });

    execSystemControlTasks();

    if (shouldReboot) {
      TaskTimeout.detach();
      logInput("Reboot requested");
      if (!dontSaveConfig) {
        saveSystemConfig();
      }
      esp_task_wdt_init(1, true);
      esp_task_wdt_add(NULL);
      while (true);
    }

    TaskSysControlHWmark = uxTaskGetStackHighWaterMark( NULL );
    vTaskDelay(25 / portTICK_PERIOD_MS);
  }

  //else delete task
  vTaskDelete( NULL );
}

#endif




void execSystemControlTasks() {

  if (IthoInit && millis() > 250) {
    IthoInit = ithoInitCheck();
  }
#if defined (HW_VERSION_TWO)
  if (!i2cStartCommands && millis() > 15000) {
    sendQueryDevicetype(i2c_result_updateweb);
    if (itho_fwversion > 0) {
      ithoInitResult = 1;
    }
    else {
      ithoInitResult = -1;
    }
    if (systemConfig.sysfirhum == 1 && ithoDeviceID == 0x1B) {
      if (itho_fwversion == 25) {
        updateSetting(63, 0);
      }
      else if (itho_fwversion == 26 || itho_fwversion == 27) {
        updateSetting(71, 0);
      }
    }

    sendQuery2400(i2c_result_updateweb); //query status info format
    sendQuery2401(i2c_result_updateweb); //query status

    i2cStartCommands = true;
  }
#endif
  if (systemConfig.itho_sendjoin > 0 && !joinSend && (ithoInitResult == 1 || millis() > 15000)) {
    joinSend = true;
    sendJoinI2C(i2c_result_updateweb);
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
    sysStatReq = writeIthoVal(ithoQueue.get_itho_speed());
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
#if defined (HW_VERSION_ONE)
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
  //request itho status every 5 sec.
  if (millis() - query2401tim >= 5000 && i2cStartCommands) {
    query2401tim = millis();
    sendQuery2401(i2c_result_updateweb);
    updateMQTTihtoStatus = true;
  }
  if (sendI2CButton) {
    sendI2CButton = false;
    sendButton(buttonValue, i2c_result_updateweb);
    i2c_result_updateweb = false;
  }  
  if (sendI2CJoin) {
    sendI2CJoin = false;
    sendJoinI2C(i2c_result_updateweb);
    i2c_result_updateweb = false;
  }
  if (sendI2CLeave) {
    sendI2CLeave = false;
    sendLeaveI2C(i2c_result_updateweb);
    i2c_result_updateweb = false;
  }  
  if (sendI2CDevicetype) {
    sendI2CDevicetype = false;
    sendQueryDevicetype(i2c_result_updateweb);
    i2c_result_updateweb = false;
  }  
  if (sendI2CStatusFormat) {
    sendI2CStatusFormat = false;
    sendQueryStatusFormat(i2c_result_updateweb);
    i2c_result_updateweb = false;
  }  
  if (sendI2CStatus) {
    sendI2CStatus = false;
    sendQueryStatus(i2c_result_updateweb);
    i2c_result_updateweb = false;
  }  
  if (send31DA) {
    send31DA = false;
    sendQuery31DA(i2c_result_updateweb);
    i2c_result_updateweb = false;
  }  
  if (send31D9) {
    send31D9 = false;
    sendQuery31D9(i2c_result_updateweb);
    i2c_result_updateweb = false;
  }  
  if (send2400) {
    send2400 = false;
    sendQuery2400(i2c_result_updateweb);
    i2c_result_updateweb = false;
  }
  if (send2401) {
    send2401 = false;
    sendQuery2401(i2c_result_updateweb);
    i2c_result_updateweb = false;
  }
  if (get2410) {
    get2410 = false;
    resultPtr2410 = sendQuery2410(i2c_result_updateweb);
    i2c_result_updateweb = false;
  }
  if (set2410) {
    setSetting2410(i2c_result_updateweb);
    set2410 = false;
    i2c_result_updateweb = false;
    getSettingsHack.once_ms(1, []() {
      getSetting(index2410, true, false);
    });
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
