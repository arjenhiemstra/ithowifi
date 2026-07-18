# ADR-0004: FreeRTOS Linear Task Chain, Flag-Polling Signaling

Status: Accepted

## Context

The firmware needs an ordered boot sequence (hardware detect → config load → network → RF → MQTT → web) and ongoing concurrent operation across those subsystems on a single-core budget. Evidence: `main/main.cpp:12` `setup()` creates one task, `TaskInit`; each subsequent task is started by the previous one (`task_init.cpp` → `startTaskConfigAndLog()` → `task_configandlog.cpp:81` → `startTaskSysControl()` → `task_syscontrol.cpp:74` → `startTaskCC1101()` → `task_cc1101.cpp:276` → `startTaskMQTT()` → `task_mqtt.cpp:46` → `startTaskWeb()`). All tasks are `xTaskCreateStaticPinnedToCore` on `CONFIG_ARDUINO_RUNNING_CORE` (single core), priority 5 except `TaskSysControl` (6) and the I2C `sniffer_task` (17). Task-to-task signaling is predominantly polled global flags (`send31D9`, `sysStatReq`, `saveSystemConfigflag`, `updateIthoMQTT`, etc.), with real RTOS primitives reserved for a small number of cases: `isrSemaphore` (CC1101 RX ISR guard), `mutexJSONLog`/`mutexWSsend` (shared buffer/log guards), `I2CManager::queueMutex` (guards a `std::deque` work queue, itself not an RTOS queue). The one `xQueueCreate` in `main/` is the I2C sniffer's ISR→task event queue (`gpio_evt_queue`, `i2c_sniffer.cpp`), not a task-to-task message bus.

## Decision

Keep the linear task-chain boot model and flag-polling signaling as the baseline concurrency pattern. Each task's loop polls the flags/state relevant to it rather than blocking on a message queue.

## Consequences

**Positive**: simple, predictable boot ordering; low RTOS primitive overhead; easy to reason about on a single core; matches the ESP32's available RAM budget better than many queues with buffered messages.

**Negative**: flag-polling introduces small latency (bounded by each task's loop period, e.g. `TaskWeb`'s `execWebTasks()` every 25ms) and requires care to avoid race conditions on flags touched by more than one task/ISR context — the existing real mutex/semaphore usage marks the spots where that mattered enough to need one. New code must not assume `xQueueCreate`-based messaging exists elsewhere in the system.

## Alternatives Considered

- **RTOS queue-based message passing between tasks**: not used for the task chain (the only queue is the sniffer's ISR→task event queue); would add queue depth/RAM sizing decisions and change the latency profile. Would need its own ADR if introduced, not a retrofit onto existing flags.
- **Multi-core task distribution**: all tasks are pinned to a single core in the current implementation; no evidence of an attempt to use the second core.

## Related Components

`main/main.cpp`, `main/tasks/*`, `main/globals.h`

## Related Issues

None tracked.

## AI Notes

Do not introduce `xQueueCreate` to solve a signaling problem without first checking whether the existing flag-polling pattern (see [.ai/common-patterns.md](../../.ai/common-patterns.md) #4) already fits — and if a queue genuinely is warranted, that's an architecture change requiring a new ADR (see trigger table in [docs/adr/README.md](README.md)), not a silent addition.
