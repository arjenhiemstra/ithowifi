#if defined (__HW_VERSION_TWO__)
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

  mqttHADiscovery();
  logInput("ha: done 1");

  logInput("Setup: done");

  vTaskDelete( NULL );
  
}

#endif
