#if defined (HW_VERSION_TWO)
void TaskInit( void * pvParameters ) {
  configASSERT( ( uint32_t ) pvParameters == 1UL );
  Ticker TaskTimeout;

  mutexLogTask = xSemaphoreCreateMutex();
  mutexJSONLog = xSemaphoreCreateMutex();
  mutexWSsend = xSemaphoreCreateMutex();
  mutexI2Ctask = xSemaphoreCreateMutex();
  
  hardwareInit();
  
  startTaskConfigAndLog();

  while (!TaskInitReady) {
    yield();
  }

  esp_task_wdt_init(40, true);
  logInput("Setup: done");

  vTaskDelete( NULL );
  
}

#endif
