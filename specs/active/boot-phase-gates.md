# Refactor Spec: boot phase gates

Status: Active
Related ADR(s): [ADR-0010](../../docs/adr/ADR-0010-boot-phase-event-group.md) (amends [ADR-0004](../../docs/adr/ADR-0004-freertos-linear-task-chain.md))

## Current State

Linear task chain — each task creates the next inside its own init:
`setup()`→TaskInit; `task_init.cpp:23`→`startTaskConfigAndLog()`; `task_configandlog.cpp:81`→`startTaskSysControl()`; `task_syscontrol.cpp:85`→`startTaskCC1101()`; `task_cc1101.cpp:275`→`startTaskMQTT()`; `task_mqtt.cpp:46`→`startTaskWeb()`; `task_web.cpp:61` sets `TaskInitReady`, spun on at `task_init.cpp:25`.

## Target State

A FreeRTOS Event Group of boot-phase bits per [ADR-0010](../../docs/adr/ADR-0010-boot-phase-event-group.md). All six tasks are created up front in one `boot()`; each `waitPhase(deps)` before its dependent init and `setPhase(own)` after. Gates: ConfigAndLog waits `HW`; SysControl waits `CONFIG`; CC1101 waits `CONFIG`; MQTT/Web wait `CONFIG|NET`; Init waits `WEB` before arming the watchdog. Each wait is bounded by `BOOT_PHASE_TIMEOUT_MS` and logs the stalled phase.

## Why

- Boot order is smeared across five files and one link (CC1101, started from SysControl) is non-obvious.
- Each task hard-codes its successor → reorder/insert edits multiple tasks.
- `startNext()` sits mid-init, so a blocked init silently stops everything downstream — no per-stage timeout, only the 60 s watchdog.
- Ordering ≠ dependencies: `TaskCC1101` needs only `CONFIG` but is serialized behind SysControl's ~4 s of `wifiInit()` delays.

## Acceptance Criteria

- [ ] External behavior unchanged — same subsystems come up in a valid dependency order; MQTT still comes up after `networkManager.initialize()` (NET); no change to API responses / MQTT payloads / config file format.
- [ ] All existing `test_native_*` suites pass unmodified.
- [ ] Boot completes on real hardware (`.246`/`.226`) with the boot log showing phases in order.

## Architecture Impact

- Touches: the task chain (boot startup only). Runtime signaling (polled flags, no queues — ADR-0004) is unchanged.
- ADR-0004's boot-startup decision is amended by ADR-0010 (noted in both).

## Risks

- A task that forgets `setPhase()` deadlocks its dependents — mitigated by the per-wait timeout (logs the stalled phase) and a boot-order native test.
- A `waitPhase` mask that under-declares a dependency (e.g. omitting `NET` for a network-using init) races/bricks — the dependency map (ADR-0010) is evidence-backed and the diff is adversarially reviewed before flashing.
- Embedded target: no user-facing revert; validate on hardware before any beta.

## Test Requirements

- [ ] Native test asserting the phase dependency invariants (a task's wait-mask ⊆ bits set by predecessors; every waited bit is set by some task; no cycle).
- [ ] Full `test_native_*` run, before/after, no assertion changes.
- [ ] Hardware boot check on `.246` and `.226`.

## Rollback Strategy

Same-PR-revertable — pure control-flow refactor, no config-format or persisted-state change. Revert the branch to restore the chain.

## Related Components

`main.cpp`, `tasks/task_init.cpp`, `tasks/task_configandlog.cpp`, `tasks/task_syscontrol.cpp`, `tasks/task_cc1101.cpp`, `tasks/task_mqtt.cpp`, `tasks/task_web.cpp`, plus a new `tasks/boot_phases.h` for the event group + helpers.
