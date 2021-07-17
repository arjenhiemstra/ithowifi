#if defined (__HW_VERSION_TWO__)

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
