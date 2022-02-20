
unsigned long lastI2CinitRequest = 0;
bool ithoInitResultLogEntry = true;

void startTaskSysControl() {
  xTaskSysControlHandle = xTaskCreateStaticPinnedToCore(
                            TaskSysControl,
                            "TaskSysControl",
                            STACK_SIZE,
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

  initSensor();

  startTaskCC1101();

  esp_task_wdt_add(NULL);

  for (;;) {
    yield();
    esp_task_wdt_reset();

    TaskTimeout.once_ms(35000, []() {
      logInput("Error: Task SysControl timed out!");
    });

    execSystemControlTasks();

    if (shouldReboot) {
      TaskTimeout.detach();
      logInput("Reboot requested");
      if (!dontSaveConfig) {
        saveSystemConfig();
      }
      delay(1000);
      ACTIVE_FS.end();
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


void execSystemControlTasks() {

  if (IthoInit && millis() > 250) {
    IthoInit = ithoInitCheck();
  }

  if (!i2cStartCommands && millis() > 15000 && (millis() - lastI2CinitRequest > 5000) ) {
    lastI2CinitRequest = millis();
    if (xSemaphoreTake(mutexI2Ctask, (TickType_t) 1000 / portTICK_PERIOD_MS) == pdTRUE) {
      sendQueryDevicetype(i2c_result_updateweb);
      xSemaphoreGive(mutexI2Ctask);
    }
    if (itho_fwversion > 0) {
      char logBuff[LOG_BUF_SIZE] = "";
      sprintf(logBuff, "I2C init: QueryDevicetype - fw:%d hw:%d", itho_fwversion, ithoDeviceID);
      logInput(logBuff);
            
      ithoInitResult = 1;
      i2cStartCommands = true;
#if defined (CVE)
      digitalWrite(ITHOSTATUS, HIGH);
#elif defined (NON_CVE)
      digitalWrite(STATUSPIN, HIGH);
#endif

      if (systemConfig.syssht30 > 0) {
        if (ithoDeviceID == 0x1B) {

          if (xSemaphoreTake(mutexI2Ctask, (TickType_t) 1000 / portTICK_PERIOD_MS) == pdTRUE) {
            i2c_result_updateweb = false;

            index2410 = 0;

            if (systemConfig.syssht30 == 1) {
              /*
                 switch itho hum setting off on every boot because
                 hum sensor setting gets restored in itho firmware after power cycle
              */
              value2410 = 0;
            }

            else if (systemConfig.syssht30 == 2) {
              /*
                 if value == 2 setting changed from 1 -> 0
                 then restore itho hum setting back to "on"
              */
              value2410 = 1;
            }
            if (itho_fwversion == 25) {
              index2410 = 63;
            }
            else if (itho_fwversion == 26 || itho_fwversion == 27) {
              index2410 = 71;
            }
            if (index2410 > 0) {
              sendQuery2410(i2c_result_updateweb);
              setSetting2410(i2c_result_updateweb);
              char logBuff[LOG_BUF_SIZE] = "";
              sprintf(logBuff, "I2C init: set hum sensor in itho firmware to: %s", value2410 ? "on" : "off" );
              logInput(logBuff);
            }
            xSemaphoreGive(mutexI2Ctask);
          }
        }
        if (systemConfig.syssht30 == 2) {
          systemConfig.syssht30 = 0;
          saveSystemConfig();
        }
      }
      if (xSemaphoreTake(mutexI2Ctask, (TickType_t) 1000 / portTICK_PERIOD_MS) == pdTRUE) {
        sendQueryStatusFormat(i2c_result_updateweb);
        char logBuff[LOG_BUF_SIZE] = "";
        sprintf(logBuff, "I2C init: QueryStatusFormat - items:%d", ithoStatus.size() );
        logInput(logBuff);
        xSemaphoreGive(mutexI2Ctask);
      }
      if (xSemaphoreTake(mutexI2Ctask, (TickType_t) 1000 / portTICK_PERIOD_MS) == pdTRUE) {
        sendQueryStatus(i2c_result_updateweb);
        logInput("I2C init: QueryStatus");
        xSemaphoreGive(mutexI2Ctask);
      }

      sendHomeAssistantDiscovery = true;
    }
    else {
      ithoInitResult = -1;
      if(ithoInitResultLogEntry) {
        ithoInitResultLogEntry = false;
        logInput("I2C init: QueryDevicetype - failed");
      }
    }
  }

  if (systemConfig.itho_sendjoin > 0 && !joinSend && ithoInitResult == 1) {
    joinSend = true;

    if (xSemaphoreTake(mutexI2Ctask, (TickType_t) 500 / portTICK_PERIOD_MS) == pdTRUE) {
      sendJoinI2C(i2c_result_updateweb);
      xSemaphoreGive(mutexI2Ctask);
      logInput("I2C init: Virtual remote join command send");
    }
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
  if (ithoQueue.ithoSpeedUpdated) {
    ithoQueue.ithoSpeedUpdated = false;
#if defined (CVE)
    sysStatReq = writeIthoVal(ithoQueue.get_itho_speed());
#endif
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
  if (!(itho_fwversion > 0) && millis() - lastVersionCheck > 60000) {
    lastVersionCheck = millis();
    if (xSemaphoreTake(mutexI2Ctask, (TickType_t) 1000 / portTICK_PERIOD_MS) == pdTRUE) {
      sendQueryDevicetype(i2c_result_updateweb);
      xSemaphoreGive(mutexI2Ctask);
    }
  }
  //request itho status every 5 sec.
  if (millis() - query2401tim >= systemConfig.itho_updatefreq * 1000UL && i2cStartCommands) {
    query2401tim = millis();
    if (xSemaphoreTake(mutexI2Ctask, (TickType_t) 500 / portTICK_PERIOD_MS) == pdTRUE) {
      sendQueryStatus(i2c_result_updateweb);
      xSemaphoreGive(mutexI2Ctask);
    }
    if (xSemaphoreTake(mutexI2Ctask, (TickType_t) 500 / portTICK_PERIOD_MS) == pdTRUE) {
      sendQuery31DA(i2c_result_updateweb);
      xSemaphoreGive(mutexI2Ctask);
    }
    if (xSemaphoreTake(mutexI2Ctask, (TickType_t) 500 / portTICK_PERIOD_MS) == pdTRUE) {
      sendQuery31D9(i2c_result_updateweb);
      xSemaphoreGive(mutexI2Ctask);
    }
    updateMQTTihtoStatus = true;
  }
  if (ithoInitResult == -1) {
    if (millis() - mqttUpdatetim >= systemConfig.itho_updatefreq * 1000UL) {
       mqttUpdatetim = millis();
       updateMQTTihtoStatus = true;
    }
  }
  if (sendI2CButton) {
    sendI2CButton = false;
    sendButton(buttonValue, i2c_result_updateweb);
    i2c_result_updateweb = false;

    xSemaphoreGive(mutexI2Ctask);
  }
  if (sendI2CTimer) {
    sendI2CTimer = false;
    sendTimer(timerValue, i2c_result_updateweb);
    i2c_result_updateweb = false;

    xSemaphoreGive(mutexI2Ctask);
  }
  if (sendI2CJoin) {
    sendI2CJoin = false;
    sendJoinI2C(i2c_result_updateweb);

    xSemaphoreGive(mutexI2Ctask);
  }
  if (sendI2CLeave) {
    sendI2CLeave = false;
    sendLeaveI2C(i2c_result_updateweb);
    i2c_result_updateweb = false;

    xSemaphoreGive(mutexI2Ctask);
  }
  if (sendI2CDevicetype) {
    sendI2CDevicetype = false;
    sendQueryDevicetype(i2c_result_updateweb);

    xSemaphoreGive(mutexI2Ctask);
  }
  if (sendI2CStatusFormat) {
    sendI2CStatusFormat = false;
    sendQueryStatusFormat(i2c_result_updateweb);

    xSemaphoreGive(mutexI2Ctask);
  }
  if (sendI2CStatus) {
    sendI2CStatus = false;
    sendQueryStatus(i2c_result_updateweb);

    xSemaphoreGive(mutexI2Ctask);
  }
  if (send31DA) {
    send31DA = false;
    sendQuery31DA(i2c_result_updateweb);

    xSemaphoreGive(mutexI2Ctask);
  }
  if (send31D9) {
    send31D9 = false;
    sendQuery31D9(i2c_result_updateweb);

    xSemaphoreGive(mutexI2Ctask);
  }
  if (send10D0) {
    send10D0 = false;
    filterReset();

    xSemaphoreGive(mutexI2Ctask);
  }  
  if (get2410) {
    get2410 = false;
    resultPtr2410 = sendQuery2410(i2c_result_updateweb);

    xSemaphoreGive(mutexI2Ctask);
  }
  if (set2410) {
    setSetting2410(i2c_result_updateweb);
    set2410 = false;

    xSemaphoreGive(mutexI2Ctask);

    getSettingsHack.once_ms(1, []() {
      getSetting(index2410, true, false, false);
    });
  }

  if (systemConfig.syssht30 == 1) {
    if (millis() - SHT3x_readout >= systemConfig.itho_updatefreq * 1000UL && (SHT3x_original || SHT3x_alternative)) {
      SHT3x_readout = millis();

      if (SHT3x_original || SHT3x_alternative) {
        if (SHT3x_original) {
          if (sht_org.readSample()) {
            Wire.endTransmission(true);
            ithoHum = sht_org.getHumidity();
            ithoTemp = sht_org.getTemperature();
          }
        }
        if (SHT3x_alternative) {
          if (sht_alt.readSample()) {
            Wire.endTransmission(true);
            ithoHum = sht_alt.getHumidity();
            ithoTemp = sht_alt.getTemperature();
          }
        }
      }


    }
  }

}
