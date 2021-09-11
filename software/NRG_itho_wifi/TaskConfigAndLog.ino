#if defined (HW_VERSION_TWO)

void startTaskConfigAndLog() {
  xTaskConfigAndLogHandle = xTaskCreateStaticPinnedToCore(
                              TaskConfigAndLog,
                              "TaskConfigAndLog",
                              STACK_SIZE_LARGE,
                              ( void * ) 1,
                              TASK_CONFIG_AND_LOG_PRIO,
                              xTaskConfigAndLog,
                              &xTaskConfigAndLogBuffer,
                              CONFIG_ARDUINO_RUNNING_CORE);
}

void TaskConfigAndLog( void * pvParameters ) {
  configASSERT( ( uint32_t ) pvParameters == 1UL );

  initFileSystem();
  logInit();
  loadSystemConfig();

  startTaskSysControl();

  esp_task_wdt_add(NULL);

  for (;;) {
    yield();
    esp_task_wdt_reset();

    TaskConfigAndLogTimeout.once_ms(1000, []() {
      logInput("Error: Task ConfigAndLog timed out!");
    });

    execLogAndConfigTasks();

    TaskConfigAndLogHWmark = uxTaskGetStackHighWaterMark( NULL );
    vTaskDelay(25 / portTICK_PERIOD_MS);
  }

  //else delete task
  vTaskDelete( NULL );
}

#endif

void execLogAndConfigTasks() {
  //Logging and config tasks
#if defined (HW_VERSION_TWO)
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
#if defined (HW_VERSION_TWO)
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

  if (formatFileSystem) {
    formatFileSystem = false;
    char logBuff[LOG_BUF_SIZE] = "";
    StaticJsonDocument<128> root;
    JsonObject systemstat = root.createNestedObject("systemstat");

    if (SPIFFS.format()) {
      systemstat["format"] = 1;
      strcpy(logBuff, "Filesystem format = success");
      dontSaveConfig = true;
    }
    else {
      systemstat["format"] = 0;
      strcpy(logBuff, "Filesystem format = failed");
    }
    jsonLogMessage(logBuff, WEBINTERFACE);
    strcpy(logBuff, "");
    char buffer[128];
    size_t len = serializeJson(root, buffer);
    notifyClients(buffer, len);
  }

}
