#if defined (__HW_VERSION_TWO__)



void TaskMQTT( void * pvParameters ) {
  configASSERT( ( uint32_t ) pvParameters == 1UL );


  mqttInit();

  startTaskWeb();

  esp_task_wdt_add(NULL);

  for (;;) {
    yield();
    esp_task_wdt_reset();

    TaskMQTTTimeout.once_ms(35000UL, []() {
      logInput("Error: Task MQTT timed out!");
    });

    execMQTTTasks();

    TaskMQTTHWmark = uxTaskGetStackHighWaterMark( NULL );
    vTaskDelay(25 / portTICK_PERIOD_MS);
  }

  //else delete task
  vTaskDelete( NULL );
}

#endif
