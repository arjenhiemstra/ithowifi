# Testing Strategy

Two independent tiers — know which one applies before writing or running tests.

## Tier 1 — Native unit tests (host machine, no hardware)

Location: `software/NRG_itho_wifi/test/test_native_*/`, PlatformIO + Unity framework, each suite has a local `mocks/` (mostly a minimal `Arduino.h` shim).

**Pattern**: each suite largely *reimplements/extracts* the pure logic from the corresponding `main/` source file rather than linking the real ESP32 sources — this keeps them decoupled from hardware/RTOS/network. When you change logic in a file that has a corresponding `test_native_*` suite, update both the source and the extracted test copy.

| Suite | Covers |
|---|---|
| `test_native_api_validation` | `WebAPIv2.cpp` validation (`processGetsettingCommands`, `processSetsettingCommands`, `processSetRFremote`) |
| `test_native_config` | `SystemConfig` defaults + JSON round-trip |
| `test_native_helpers` | `parseHexString`, `compareVersions`, `round` |
| `test_native_json` | JSend response construction, command/param validation |
| `test_native_mqtt_cmd_parse` | MQTT payload type coercion |
| `test_native_mqtt_response` | `mqttSendResponse` JSON structure |
| `test_native_queue` | `IthoQueue` add/sort/timer/clear/fallback/serialize |
| `test_native_remote` | `IthoRemote` slot management |
| `test_native_rf` | RF end-byte selection, command name lookup |
| `test_native_speed_parse` | Speed/timer parsing (`ithoSetSpeed`, `ithoSetTimer`, `ithoExecCommand`) |
| `test_native_uuid` | `uuidUnparse`/`uuidParse` |
| `test_native_wifi_config` | `WifiConfig` serialization round-trip |

Run: `pio test -e native_test` from `software/NRG_itho_wifi/` (see [AGENTS.md](../AGENTS.md) for the `pio` binary path). This is what CI runs on every push/PR (`.github/workflows/ci.yml`).

## Tier 2 — Live-hardware integration tests (pytest)

Location: `tests/api/`, `tests/mqtt/` (repo root, **not** under `software/`). Requires a real, reachable ithowifi device:

```bash
pytest tests/api --device <device-ip>
# or
ITHO_DEVICE=<device-ip> pytest tests/api tests/mqtt
```

Covers: REST endpoints, OpenAPI spec completeness/consistency, websocket, config persistence, security/auth, RF CO2/demand commands, fan commands, remote config, stability, reboot gating, response format. Not run in CI (no hardware available there) — run manually against a bench device before release.

## When adding a feature

- New pure logic in `WebAPIv2.cpp`/similar → add/extend a `test_native_*` suite.
- New REST endpoint or behavior visible externally → add a `tests/api/test_*.py` case, run manually against hardware.
- New MQTT topic/command → mirror in `test_native_mqtt_*` and `tests/mqtt/`.

## Related

[.ai/architecture-summary.md](architecture-summary.md) · [docs/adr/ADR-0009-dual-tier-testing-strategy.md](../docs/adr/ADR-0009-dual-tier-testing-strategy.md) · [specs/templates/feature-spec.md](../specs/templates/feature-spec.md) (test requirements section)
