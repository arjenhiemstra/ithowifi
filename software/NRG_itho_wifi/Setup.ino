
#if defined (__HW_VERSION_ONE__)
void setup() {

#if defined (ENABLE_SERIAL)
  Serial.begin(115200);
  Serial.flush();
  delay(100);
#endif

  hardwareInit();

  initFileSystem();

  logInit();

  wifiInit();

  loadSystemConfig();

#if defined(ENABLE_SHT30_SENSOR_SUPPORT)
  initSensor();
#endif

  mqttInit();

  ArduinoOTAinit();

  websocketInit();

  webServerInit();

  MDNSinit();

#if defined (__HW_VERSION_TWO__)
  Ticker TaskTimeout;
  CC1101TaskStart = true;
  delay(500);
#endif

  logInput("Setup: done");
}
#elif defined (__HW_VERSION_TWO__)
void setup() {

#if defined (ENABLE_SERIAL)
  Serial.begin(115200);
  Serial.flush();
  delay(100);
#endif

  xTaskInitHandle = xTaskCreateStaticPinnedToCore(
                      TaskInit,
                      "TaskInit",
                      STACK_SIZE,
                      ( void * ) 1,
                      TASK_MAIN_PRIO,
                      xTaskInitStack,
                      &xTaskInitBuffer,
                      CONFIG_ARDUINO_RUNNING_CORE);

}

void startTaskCC1101() {
  xTaskCC1101Handle = xTaskCreateStaticPinnedToCore(
                        TaskCC1101,
                        "TaskCC1101",
                        STACK_SIZE,
                        ( void * ) 1,
                        TASK_CC1101_PRIO,
                        xTaskCC1101Stack,
                        &xTaskCC1101Buffer,
                        CONFIG_ARDUINO_RUNNING_CORE);
}
void startTaskMQTT() {
  xTaskMQTTHandle = xTaskCreateStaticPinnedToCore(
                      TaskMQTT,
                      "TaskMQTT",
                      STACK_SIZE,
                      ( void * ) 1,
                      TASK_MQTT_PRIO,
                      xTaskMQTTStack,
                      &xTaskMQTTBuffer,
                      CONFIG_ARDUINO_RUNNING_CORE);
}
void startTaskConfigAndLog() {
  xTaskConfigAndLogHandle = xTaskCreateStaticPinnedToCore(
                              TaskConfigAndLog,
                              "TaskConfigAndLog",
                              STACK_SIZE_LARGE,
                              ( void * ) 1,
                              TASK_CONFIG_AND_LOG_PRIO,
                              xTaskConfigAndLog,
                              &xTaskConfigAndLogBuffer,
                              CONFIG_ARDUINO_RUNNING_CORE);
}
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
void startTaskSysControl() {
  xTaskSysControlHandle = xTaskCreateStaticPinnedToCore(
                            TaskSysControl,
                            "TaskSysControl",
                            STACK_SIZE_LARGE,
                            ( void * ) 1,
                            TASK_SYS_CONTROL_PRIO,
                            xTaskSysControlStack,
                            &xTaskSysControlBuffer,
                            CONFIG_ARDUINO_RUNNING_CORE);
}

#endif
