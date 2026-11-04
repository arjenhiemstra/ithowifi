#include <Arduino.h>
#include "globals.h"
#include "tasks/boot_phases.h"
#include "tasks/task_init.h"
#include "tasks/task_configandlog.h"
#include "tasks/task_syscontrol.h"
#include "tasks/task_cc1101.h"
#include "tasks/task_mqtt.h"
#include "tasks/task_web.h"

#define TASK_MAIN_PRIO 5

// locals
TaskHandle_t xTaskInitHandle = NULL;

void setup()
{
  // put your setup code here, to run once:

#if defined(ENABLE_SERIAL)
  Serial.begin(115200);
  Serial.flush();
  delay(100);
#endif

  bootPhasesInit();

  // All boot tasks are created up front; each gates on the boot phases it
  // depends on (ADR-0010). Creation order is not significant — the event group
  // enforces the real dependency order (HW -> CONFIG -> NET, RF off CONFIG).
  // Dynamic: TaskInit deletes itself after boot, which only returns the stack to the heap this way
  xTaskCreatePinnedToCore(
      TaskInit,
      "TaskInit",
      STACK_SIZE,
      (void *)1,
      TASK_MAIN_PRIO,
      &xTaskInitHandle,
      CONFIG_ARDUINO_RUNNING_CORE);
  startTaskConfigAndLog();
  startTaskSysControl();
  startTaskCC1101();
  startTaskMQTT();
  startTaskWeb();

  // loopTask has nothing to do. Left running it starves the idle task of this core,
  // and that idle task is what frees the memory of deleted tasks pinned here.
  vTaskDelete(NULL);
}

void loop()
{
}
