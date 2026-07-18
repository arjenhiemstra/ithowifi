# Context Map

Minimal-token lookup table of the highest-value files/classes/tasks/interfaces. Prefer this over re-scanning the tree.

## Boot / tasks

| Task | File |
|---|---|
| TaskInit | `main/tasks/task_init.cpp` |
| TaskConfigAndLog | `main/tasks/task_configandlog.cpp` |
| TaskSysControl | `main/tasks/task_syscontrol.cpp` |
| TaskCC1101 | `main/tasks/task_cc1101.cpp` |
| TaskMQTT | `main/tasks/task_mqtt.cpp` |
| TaskWeb | `main/tasks/task_web.cpp` |
| sniffer_task | `main/ithodevice/i2c_sniffer.cpp` |
| Entry point | `main/main.cpp` (`setup()`, `loop()`) |

## Managers (global instances)

| Instance | File |
|---|---|
| `hardwareManager` | `main/managers/HardwareManager.h` |
| `i2cManager` | `main/managers/I2CManager.h` |
| `networkManager` | `main/managers/NetworkManager.h` |
| `rfManager` | `main/managers/RFManager.h` |
| `secureWebCommLite` | `main/managers/SecureWebCommLite.h` |

## Critical interfaces

| Interface | File |
|---|---|
| Device table | `main/ithodevice/IthoDevice.h` (`ithoDeviceType`), `IthoDevice.cpp` (`ithoDevices[]`) |
| Device status parsing | `main/ithodevice/IthoStatus.cpp` |
| Device settings get/set | `main/ithodevice/IthoSettings.cpp` |
| Virtual remote commands | `main/ithodevice/IthoVirtualRemoteCmd.cpp` |
| I2C transport | `main/ithodevice/i2c_esp32.cpp` |
| REST v2 logic | `main/api/WebAPIv2.h` / `.cpp` |
| REST v2 routing | `main/api/WebAPIv2Rest.cpp` |
| REST v1 (legacy) | `main/api/WebAPIv1.cpp` |
| MQTT command dispatch | `main/api/MqttAPI.cpp` (`mqttCallback`) |
| API response envelope | `main/api/ApiResponse.h` |
| CC1101 low-level driver | `main/cc1101/CC1101.h` |
| Itho RF protocol layer | `main/cc1101/IthoCC1101.h` |

## Config flow

`main/config/SystemConfig.h`, `WifiConfig.h`, `LogConfig.h`, `HADiscConfig.h`, `IthoRemote.h` → loaded/saved via free functions declared in `main/config/Config.h`, backed by LittleFS JSON (all config types are also snapshotted to NVS before a flash repartition and restored on next boot). Triggered by polled flags (`saveSystemConfigflag`, `resetWifiConfigflag`) checked in `TaskConfigAndLog`/`TaskSysControl`.

## Build-generated (don't hand-edit)

`main/webroot/*_gz.h` ← `software/NRG_itho_wifi/build_script.py` ← `main/webroot_source/*`.

## Test entry points

- Native: `software/NRG_itho_wifi/test/test_native_*` — see [.ai/testing-strategy.md](testing-strategy.md)
- Live-hardware: `tests/api/`, `tests/mqtt/` (repo root)

## Full detail

[docs/architecture/repository-map.md](../docs/architecture/repository-map.md)
