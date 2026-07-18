# GitHub Copilot Instructions — ithowifi

ESP32 firmware (PlatformIO, `arduino`+`espidf`) bridging Itho Daalderop ventilation units to WiFi/MQTT/REST. Full context: [AGENTS.md](../AGENTS.md), [docs/architecture/repository-map.md](../docs/architecture/repository-map.md), [.ai/](../.ai/).

## Architecture overview

- Boot: linear FreeRTOS task chain `TaskInit → TaskConfigAndLog → TaskSysControl → TaskCC1101 → TaskMQTT → TaskWeb`, single core; task-chain signaling is polled global flags plus a few real mutexes/semaphores (the only FreeRTOS queue is the I2C sniffer's ISR event queue).
- Hardware/network/RF/I2C access via global manager singletons (`main/managers/*`).
- Device support is a data table (`ithoDevices[]`), not class inheritance.
- REST v2: pure logic (`processXxx`, `main/api/WebAPIv2.cpp`) separate from HTTP routing (`handleXxx`, `main/api/WebAPIv2Rest.cpp`); MQTT (`main/api/MqttAPI.cpp`) reuses the same logic.
- Config: LittleFS JSON + NVS backup (remotes only).

Decision record for all of the above: [docs/adr/](../docs/adr/) (ADR-0002 through ADR-0009).

## Naming conventions

| Pattern | Used for |
|---|---|
| `TaskX` | FreeRTOS task entry functions |
| `processXxx` | Pure-logic API handlers |
| `handleGetXxx` / `handlePostXxx` | AsyncWebServer route bindings |
| `<name>Manager` global instance | Manager singletons |
| `load*Config` / `save*Config` | Config persistence functions |

## Design principles

- Reuse the existing pattern before introducing a new one — see [.ai/common-patterns.md](../.ai/common-patterns.md).
- `-Os` build target: this is a flash/RAM-constrained embedded device, not a server — avoid heavyweight abstractions.
- Never edit `software/lib/*` (vendored libraries) or hand-edit `main/webroot/*_gz.h` (generated).
- Preserve the logic/routing split in `main/api/` and the data-table shape in `main/ithodevice/` — both exist specifically to keep the codebase testable and extensible.

## Repository navigation

| Path | Contents |
|---|---|
| `software/NRG_itho_wifi/main/` | Firmware source |
| `software/NRG_itho_wifi/main/tasks/` | FreeRTOS task entry points |
| `software/NRG_itho_wifi/main/managers/` | Hardware/network/RF/I2C singletons |
| `software/NRG_itho_wifi/main/api/` | REST v2, MQTT command dispatch, OpenAPI |
| `software/NRG_itho_wifi/main/ithodevice/` | Device data model, status/settings, I2C transport |
| `software/NRG_itho_wifi/main/cc1101/` | CC1101 RF driver |
| `software/NRG_itho_wifi/main/config/` | Config structs + persistence |
| `software/NRG_itho_wifi/test/test_native_*/` | Host-side Unity unit tests |
| `tests/api/`, `tests/mqtt/` | Live-hardware pytest integration tests |
| `software/lib/` | Vendored libraries — do not touch |

Full map with diagrams: [docs/architecture/repository-map.md](../docs/architecture/repository-map.md).

## Testing requirements

- New pure logic → add/extend a `test_native_*` suite (`software/NRG_itho_wifi/test/`), run via `pio test -e native_test` from `software/NRG_itho_wifi/` (also CI-gated).
- New externally-observable behavior (REST/MQTT/websocket) → add a `tests/api/` or `tests/mqtt/` pytest case; these require real hardware (`--device`/`ITHO_DEVICE`) and are **not** run in CI — note in the PR that manual verification is needed.

Detail: [.ai/testing-strategy.md](../.ai/testing-strategy.md).

## Documentation update requirements

Any architectural change (task structure, manager shape, device model, REST/MQTT surface, config format, hardware abstraction) **must**:
1. Have a spec in `specs/active/` before implementation (templates in [specs/templates/](../specs/templates/)).
2. Update the matching ADR in [docs/adr/](../docs/adr/) — check the trigger table in [docs/adr/README.md](../docs/adr/README.md).
3. Update [docs/architecture/repository-map.md](../docs/architecture/repository-map.md) / [.ai/architecture-summary.md](../.ai/architecture-summary.md) if they become stale.

Do not duplicate content that already exists in `docs/architecture/repository-map.md` or `.ai/*.md` — link to it instead. Full rule set: [.ai/ai-instructions.md](../.ai/ai-instructions.md).
