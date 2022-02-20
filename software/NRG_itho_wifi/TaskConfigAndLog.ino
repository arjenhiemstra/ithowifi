

void startTaskConfigAndLog() {
  xTaskConfigAndLogHandle = xTaskCreateStaticPinnedToCore(
                              TaskConfigAndLog,
                              "TaskConfigAndLog",
                              STACK_SIZE,
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

    TaskConfigAndLogTimeout.once_ms(3000, []() {
      logInput("Error: Task ConfigAndLog timed out!");
    });

    execLogAndConfigTasks();

    TaskConfigAndLogHWmark = uxTaskGetStackHighWaterMark( NULL );
    vTaskDelay(25 / portTICK_PERIOD_MS);
  }

  //else delete task
  vTaskDelete( NULL );
}


void execLogAndConfigTasks() {
  //Logging and config tasks
  if (saveSystemConfigflag) {
    saveSystemConfigflag = false;
    if (saveSystemConfig()) {
      logMessagejson("System settings saved", WEBINTERFACE);
    }
    else {
      logMessagejson("Failed saving System settings: Unable to write config file", WEBINTERFACE);
    }
  }
  if (saveWifiConfigflag) {
    saveWifiConfigflag = false;
    if (saveWifiConfig()) {
      logMessagejson("Wifi settings saved, reboot the device", WEBINTERFACE);
    }
    else {
      logMessagejson("Failed saving Wifi settings: Unable to write config file", WEBINTERFACE);
    }
  }

  if (saveRemotesflag) {
    saveRemotesflag = false;
    DelayedSave.once_ms(150, []() {
      saveRemotesConfig();
      jsonWsSend("remotes");
    } );
  }
  if (saveVremotesflag) {
    saveVremotesflag = false;
    DelayedSave.once_ms(150, []() {
      saveVirtualRemotesConfig();
      jsonWsSend("vremotes");
    } );
  }  
  if (resetWifiConfigflag) {
    resetWifiConfigflag = false;
    if (resetWifiConfig()) {
      logMessagejson("Wifi settings restored, reboot the device", WEBINTERFACE);
    }
    else {
      logMessagejson("Failed restoring Wifi settings, please try again", WEBINTERFACE);
    }
  }
  if (resetSystemConfigflag) {
    resetSystemConfigflag = false;
    if (resetSystemConfig()) {
      logMessagejson("System settings restored, reboot the device", WEBINTERFACE);
    }
    else {
      logMessagejson("Failed restoring System settings, please try again", WEBINTERFACE);
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

    if (ACTIVE_FS.format()) {
      systemstat["format"] = 1;
      notifyClients(root.as<JsonObjectConst>());
      strcpy(logBuff, "Filesystem format done");
      logMessagejson(logBuff, WEBINTERFACE);
      strcpy(logBuff, "Device rebooting, connect to accesspoint to setup the device");
      logMessagejson(logBuff, WEBINTERFACE);      
      esp_task_wdt_init(1, true);
      esp_task_wdt_add(NULL);
      while (true);
    }
    else {
      systemstat["format"] = 0;
      strcpy(logBuff, "Unable to format");
      logMessagejson(logBuff, WEBINTERFACE);
      notifyClients(root.as<JsonObjectConst>());
    }
    strcpy(logBuff, "");  

  }

}
