# ADR-0003: PlatformIO + Dual Arduino/ESP-IDF Framework

Status: Accepted

## Context

The project targets ESP32 and needs both the convenience of the Arduino ecosystem (widely-used libraries: `AsyncTCP`, `ESPAsyncWebServer`, `PubSubClient`, `ArduinoJson`, `ArduinoLog`) and direct ESP-IDF access for low-level features (I2C master/slave transport, task watchdog configuration, `esp_littlefs`). Evidence: `software/NRG_itho_wifi/platformio.ini` declares `framework = arduino, espidf` and `platform = platformio/espressif32 @ ~6.12.0`; `main/ithodevice/i2c_esp32.cpp` calls raw ESP-IDF `i2c_master_*`/`i2c_slave_*` APIs while `main/tasks/task_web.cpp` and friends use Arduino-style `WiFi`/`Serial`.

## Decision

Use PlatformIO as the build system, with the `arduino, espidf` dual framework, pinned library versions via `lib_deps` (mostly GitHub commit/tag pins, e.g. `ArduinoJson.git#v7.4.3`, `ESPAsyncWebServer.git#v3.10.0`), and multiple build environments (`dev`, `beta`, `release`, `debug`, `native_test`) sharing a common `project_base` config.

## Consequences

**Positive**: access to both ecosystems; native_test env allows host-side unit testing without hardware; pinned lib_deps give reproducible builds.

**Negative**: dual-framework mixing requires care — Arduino and ESP-IDF each have their own conventions for tasks/logging/etc; `lib_ldf_mode = chain+` and pinned forks (e.g. `arjenhiemstra/ArduinoNvs.git`, `jclds139/FS_FilePrint.git`) indicate some libraries needed project-specific patches, increasing maintenance surface vs upstream.

## Alternatives Considered

- **Pure ESP-IDF (no Arduino)**: would drop the convenience of `ESPAsyncWebServer`/`PubSubClient`/`ArduinoJson` ecosystem, requiring reimplementation or ESP-IDF-native equivalents — not evidenced as attempted.
- **Pure Arduino (no ESP-IDF)**: would lose direct access to I2C master/slave dual-mode and fine-grained task watchdog control used in `i2c_esp32.cpp`/`task_init.cpp`.

## Related Components

`software/NRG_itho_wifi/platformio.ini`, `main/ithodevice/i2c_esp32.cpp`, `main/tasks/task_init.cpp`

## Related Issues

None tracked.

## AI Notes

When adding a dependency, prefer an existing `lib_deps` pin or a well-maintained Arduino-compatible library over introducing a third framework/build system. Don't propose migrating off PlatformIO without an ADR superseding this one.
