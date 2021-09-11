#if defined (HW_VERSION_TWO)

void startTaskWeb() {
  xTaskWebHandle = xTaskCreateStaticPinnedToCore(
                     TaskWeb,
                     "TaskWeb",
                     STACK_SIZE_LARGE,
                     ( void * ) 1,
                     TASK_WEB_PRIO,
                     xTaskWebStack,
                     &xTaskWebBuffer,
                     CONFIG_ARDUINO_RUNNING_CORE);
}

void TaskWeb( void * pvParameters ) {
  configASSERT( ( uint32_t ) pvParameters == 1UL );
  Ticker TaskTimeout;

  ArduinoOTAinit();

  websocketInit();

  webServerInit();

  MDNSinit();

  TaskInitReady = true;

  esp_task_wdt_add(NULL);
  
  for (;;) {
    yield();
    esp_task_wdt_reset();

    TaskTimeout.once_ms(1000, []() {
      logInput("Error: Task Web timed out!");
    });

    execWebTasks();

    TaskWebHWmark = uxTaskGetStackHighWaterMark( NULL );
    vTaskDelay(25 / portTICK_PERIOD_MS);
  }

  //else delete task
  vTaskDelete( NULL );
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
