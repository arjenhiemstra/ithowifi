#if defined (HW_VERSION_TWO)
void TaskInit( void * pvParameters ) {
  configASSERT( ( uint32_t ) pvParameters == 1UL );
  Ticker TaskTimeout;

  mutexLogTask = xSemaphoreCreateMutex();
  mutexJSONLog = xSemaphoreCreateMutex();
  mutexWSsend = xSemaphoreCreateMutex();

  hardwareInit();
  
  startTaskConfigAndLog();

  while (!TaskInitReady) {
    yield();
  }

#if defined (ENABLE_SERIAL)
  Serial.println("Setup: done");
#endif

  esp_task_wdt_init(40, true);
  logInput("Setup: done");

  vTaskDelete( NULL );
  
}

#endif
