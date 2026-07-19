# ADR-0010: Boot Sequencing — Event-Group Phase Gates

Status: Accepted (amends ADR-0004 for boot startup; runtime signaling in ADR-0004 unchanged)

## Context

Boot today is a linear task chain: each task creates the next inside its own init, so the order is spread across five files — `main.cpp` creates `TaskInit`; `task_init.cpp:23` → `startTaskConfigAndLog()`; `task_configandlog.cpp:81` → `startTaskSysControl()`; `task_syscontrol.cpp:85` → `startTaskCC1101()`; `task_cc1101.cpp:275` → `startTaskMQTT()`; `task_mqtt.cpp:46` → `startTaskWeb()`; `task_web.cpp:61` sets `TaskInitReady`, which `TaskInit` spins on (`task_init.cpp:25` `while (!TaskInitReady) yield()`).

Problems this causes:
- **Order lives in five files** and one link is non-obvious (CC1101 is started from SysControl, not where you'd look).
- **Each task hard-codes its successor** — reordering or inserting a stage means editing multiple tasks.
- **The `startNext()` call sits mid-init**, so if any init before it blocks, everything downstream silently never starts — there is no per-stage timeout, only the 60 s task watchdog.
- **Ordering is conflated with dependencies.** The real dependency graph is only three levels deep, and one task is serialized for no reason: `TaskCC1101` needs only loaded config (`systemConfig`/`logConfig`, and it loads its own remotes via `loadRemotesConfig("flash")`, `task_cc1101.cpp:325`) — yet it runs *after* `TaskSysControl`'s ~4 s of `wifiInit()` `delay()`s purely because of the chain.

Verified dependency graph (evidence in `Related Components`):

| Task | Real dependency | Produces |
|---|---|---|
| TaskInit | — | hardware, mutexes (HW) |
| TaskConfigAndLog | HW (mutexes, filesystem) | config loaded + logging (CONFIG) |
| TaskSysControl | CONFIG | `networkManager.initialize()` + WiFi (NET) |
| TaskCC1101 | CONFIG only | RF ready (RF) |
| TaskMQTT | CONFIG + NET | — |
| TaskWeb | CONFIG + NET | boot complete (WEB) |

The one hidden edge the chain masks: `mqttClient` is constructed against `networkManager.standardClient` (`task_mqtt.cpp:14`), which `TaskSysControl` sets up — so **MQTT genuinely depends on NET**, not just on running "after CC1101".

## Decision

Replace the chain with a single FreeRTOS **Event Group** of boot-phase bits. All tasks are created up front in one `boot()` in `setup()`; each task waits (bounded, with a timeout) on the phase bits it depends on before its dependent init, and sets its own bit when that init completes.

Phase bits and gates:

| Task | waits for | sets |
|---|---|---|
| TaskInit | — | `PHASE_HW`; then waits `PHASE_WEB` before arming the watchdog |
| TaskConfigAndLog | `PHASE_HW` | `PHASE_CONFIG` |
| TaskSysControl | `PHASE_CONFIG` | `PHASE_NET` |
| TaskCC1101 | `PHASE_CONFIG` | `PHASE_RF` |
| TaskMQTT | `PHASE_CONFIG \| PHASE_NET` | — |
| TaskWeb | `PHASE_CONFIG \| PHASE_NET` | `PHASE_WEB` |

`waitPhase(bits)` uses `xEventGroupWaitBits(..., pdMS_TO_TICKS(BOOT_PHASE_TIMEOUT_MS))` and, on timeout, logs which phase stalled instead of spinning forever. `PHASE_WEB` replaces the `TaskInitReady` flag.

## Consequences

**Positive**: boot order + dependencies live in one place (the `boot()` creation list plus each task's `waitPhase(...)` first line); tasks no longer reference each other, so reordering/inserting a stage is a mask change; a stalled stage is logged by name with a timeout instead of hanging silently; `TaskCC1101` now inits off `PHASE_CONFIG` in parallel with SysControl's network bring-up rather than behind its ~4 s of `delay()`s.

**Negative**: one event group + a phase enum to maintain; a task that forgets to `setPhase()` deadlocks its dependents (mitigated by the per-wait timeout + the boot-order native test). Boot is still single-core cooperative, so "parallel" init is interleaving, not true concurrency.

## Alternatives Considered

- **Centralized ordered boot (lift `startTaskX()` into one `boot()`, keep it sequential):** simpler, but doesn't express dependencies or add per-stage timeouts. Kept as the first migration step toward this.
- **Boot state-machine / orchestrator:** more robust (retries, observable boot phase) but a heavier new component than warranted for six tasks.
- **Table-driven init registry:** declarative and fits the `ithoDevices[]` idiom, but more machinery than six tasks need today.

## Related Components

`main.cpp` (`setup()`), `tasks/task_init.cpp`, `tasks/task_configandlog.cpp`, `tasks/task_syscontrol.cpp` (`networkManager.initialize()` :61), `tasks/task_cc1101.cpp` (config-only init :280-348), `tasks/task_mqtt.cpp` (`mqttClient(networkManager.standardClient)` :14), `tasks/task_web.cpp` (`TaskInitReady` :61), `managers/NetworkManager.cpp`.

## Related Issues

Supersedes the boot-startup mechanism of [ADR-0004](ADR-0004-freertos-linear-task-chain.md) (its runtime flag-polling / no-task-queue decision is unchanged).

## AI Notes

Boot dependencies are expressed as `waitPhase(...)`/`setPhase(...)`, not by call order. To add or reorder a stage: create the task in `boot()`, wait on the phases it needs, set its own phase when ready — do not reintroduce `startTaskX()`-from-another-task chaining. Every phase a task waits on must be set by some task, or it deadlocks (the timeout will log it). A task's `waitPhase` mask must reflect what its init actually reads (grep for cross-task globals — e.g. anything touching `networkManager` needs `PHASE_NET`).

**`waitPhase(...)` must be a task's FIRST action — before any logging or other dependency use.** `sys_log()` dereferences `syslog_queueSemaphore`, which is created in `TaskConfigAndLog` (`PHASE_CONFIG`); a `D_LOG`/`I_LOG`/etc. before the wait crashes with `StoreProhibited` (this bricked boot once — `TaskCC1101` logged "started" before waiting). For the same reason `waitPhase`'s own timeout message uses `ESP_LOGW`, not `sys_log`, so it is safe before `PHASE_CONFIG`.
