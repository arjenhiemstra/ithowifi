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

  logInput("Setup: done");

  vTaskDelete( NULL );
  
//  while (1) {
//
//    delay(0);
//    yield();
//
//    TaskTimeout.once_ms(100, []() {
//      logInput("Error: MainTask timed out!");
//    });
//
//    TaskInitHWmark = uxTaskGetStackHighWaterMark( NULL );
//    vTaskDelay(10 / portTICK_PERIOD_MS);
//  }
}

#endif
