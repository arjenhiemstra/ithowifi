#if defined (__HW_VERSION_TWO__)


void TaskConfigAndLog( void * pvParameters ) {
  configASSERT( ( uint32_t ) pvParameters == 1UL );

  initFileSystem();
  logInit();
  loadSystemConfig();

  startTaskSysControl();

  esp_task_wdt_init(5, true);
  esp_task_wdt_add(NULL);

  for (;;) {
    yield();
    esp_task_wdt_reset();

    TaskConfigAndLogTimeout.once_ms(1000, []() {
      logInput("Error: Task ConfigAndLog timed out!");
    });

    execLogAndConfigTasks();

    TaskConfigAndLogHWmark = uxTaskGetStackHighWaterMark( NULL );
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }

  //else delete task
  vTaskDelete( NULL );
}

#endif
