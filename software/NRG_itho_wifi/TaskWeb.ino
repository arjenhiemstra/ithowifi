#if defined (__HW_VERSION_TWO__)

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
