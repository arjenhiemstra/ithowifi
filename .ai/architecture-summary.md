# Architecture Summary

> Condensed for token-efficient lookup. Full detail + diagrams: [docs/architecture/repository-map.md](../docs/architecture/repository-map.md).

## Boot chain (linear, not a queue pipeline)

`TaskInit → TaskConfigAndLog → TaskSysControl → TaskCC1101 → TaskMQTT → TaskWeb`

Each task starts the next, then enters its own loop. Single core. Cross-task signaling is mostly **polled global flags** plus a few real mutexes/semaphores (`isrSemaphore`, `mutexJSONLog`, `mutexWSsend`, `I2CManager::queueMutex`); the only FreeRTOS queue is the I2C sniffer's ISR event queue (`gpio_evt_queue`).

| Task | File | Priority |
|---|---|---|
| TaskInit | `main/tasks/task_init.cpp` | 5 |
| TaskConfigAndLog | `main/tasks/task_configandlog.cpp` | 5 |
| TaskSysControl | `main/tasks/task_syscontrol.cpp` | 6 (highest of the chain) |
| TaskCC1101 | `main/tasks/task_cc1101.cpp` | 5 |
| TaskMQTT | `main/tasks/task_mqtt.cpp` | 5 |
| TaskWeb | `main/tasks/task_web.cpp` | 5 |
| sniffer_task | `main/ithodevice/i2c_sniffer.cpp` | 17 |

## Managers (`main/managers/`) — global singleton instances, not classes to re-instantiate

`hardwareManager`, `i2cManager`, `networkManager`, `rfManager` (owns CC1101 radio), plus flag-based `SensorManager`/`WiFiConnectionManager` (no class), `secureWebCommLite`.

## Device model (`main/ithodevice/`) — data table, not inheritance

`ithoDeviceType ithoDevices[]` (`IthoDevice.cpp:35`) — one row per device model, populated from per-device headers in `main/ithodevice/devices/`. Runtime lookup via `getDevicePtr(deviceGroup, deviceID)`. To add a device: add a row + label header, don't create a subclass.

## API layer (`main/api/`)

Deliberate split for testability:
- `WebAPIv2.cpp` — pure logic, `processXxx(...)` functions, framework-agnostic, JSON in/out
- `WebAPIv2Rest.cpp` — `AsyncWebServer` route binding only (`handleGetXxx`/`handlePostXxx`)
- `MqttAPI.cpp` — `mqttCallback` parses `<base>/cmd`, dispatches to the same `processXxx` logic

Preserve this split — it's what lets `test_native_api_validation` test logic without ESP32/network stack.

## Config (`main/config/`)

LittleFS JSON files; all config types are snapshotted to NVS before a flash repartition (`backupAllConfigs()`) and restored on next boot. Config classes: `SystemConfig`, `WifiConfig`, `LogConfig`, `HADiscConfig`, `IthoRemote`.

## RF (`main/cc1101/`)

`CC1101` = generic SPI register driver. `IthoCC1101 : protected CC1101` = Itho protocol layer. RX is interrupt-driven (`ITHOinterrupt`, `IRAM_ATTR`, `main/tasks/task_cc1101.cpp:36`) — decode happens in ISR context via `isrSemaphore`, actual packet dispatch happens in `TaskCC1101`'s loop.

## Testing — two tiers

1. Native Unity (`software/NRG_itho_wifi/test/test_native_*`) — host-side, pure-logic extraction, no hardware
2. pytest (`tests/api/`, `tests/mqtt/`) — **live hardware required** (`--device`/`ITHO_DEVICE` env var)

Full detail: [.ai/testing-strategy.md](testing-strategy.md).

## Related

[.ai/common-patterns.md](common-patterns.md) · [.ai/coding-standards.md](coding-standards.md) · [.ai/context-map.md](context-map.md) · [docs/adr/README.md](../docs/adr/README.md)
