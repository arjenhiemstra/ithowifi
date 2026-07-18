# Coding Standards

Derived from actual repo conventions — do not introduce conflicting styles.

## Build flags (enforced by `platformio.ini`)

- C++20: `-std=gnu++2a`; C17: `-std=gnu17`
- `-Wall -Wextra -Wunused-but-set-variable -Wdeprecated-declarations`, `-Wno-error=format` (warnings on, format warnings non-fatal)
- `-Os` size optimization — this is a flash/RAM-constrained ESP32 target, avoid large static buffers or heavy templated code
- `framework = arduino, espidf` — both APIs are available; code mixes Arduino-style (`Serial`, `WiFi`) and raw ESP-IDF calls (`i2c_master_*`, task watchdog)

## Naming patterns (follow existing, don't invent new ones)

| Pattern | Used for | Example |
|---|---|---|
| `TaskX` | FreeRTOS task entry functions | `TaskCC1101`, `TaskMQTT` |
| `processXxx` | Pure-logic API handlers (framework-agnostic) | `processGetCommands`, `processSetRFremote` |
| `handleGetXxx` / `handlePostXxx` | AsyncWebServer route bindings | `handleGetSpeed`, `handlePostCommand` |
| `<manager>Manager` global instance | Manager singletons | `hardwareManager`, `i2cManager`, `rfManager` |
| `load*Config` / `save*Config` | Config persistence free functions | `loadWifiConfig`, `saveSystemConfig` |

## Patterns to preserve

- **Logic/routing split**: business logic in `processXxx` (in `WebAPIv2.cpp`), HTTP binding in `handleXxx` (in `WebAPIv2Rest.cpp`). Never inline business logic directly in an `AsyncWebServer` route lambda — it breaks native testability. See [.ai/common-patterns.md](common-patterns.md).
- **Data-table device model**: new Itho device support = new row in `ithoDevices[]` + a label header in `main/ithodevice/devices/`, not a new C++ class.
- **Manager singletons**: access hardware/network/RF/I2C state via the existing global instance (`rfManager`, `i2cManager`, etc.), don't construct new instances.
- **ISR safety**: code in `ITHOinterrupt` (`task_cc1101.cpp`) is `IRAM_ATTR` and runs in interrupt context — keep it minimal, no blocking calls, no `Serial.print`.

## Response format

REST v2 uses JSend-style responses via `ApiResponse` (`main/api/ApiResponse.h`) — reuse it, don't hand-roll new response JSON shapes. `api_response_status_t` is the standard return type.

## Vendored code

Never modify anything under `software/lib/`. If a vendored library needs a fix, pin a different version/commit in `platformio.ini` `lib_deps` instead.

## Full architecture context

[.ai/architecture-summary.md](architecture-summary.md) · [docs/adr/ADR-0006-rest-api-v2-logic-routing-split.md](../docs/adr/ADR-0006-rest-api-v2-logic-routing-split.md)
