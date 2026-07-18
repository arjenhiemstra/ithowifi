# Prompt Template — Test Generation

## Decide which tier first

See [.ai/testing-strategy.md](../testing-strategy.md) for the full tier breakdown.

- Pure logic (parsing, validation, response building, no hardware/network) → **native Unity test**, `software/NRG_itho_wifi/test/test_native_*`.
- Externally observable behavior (REST response shape, MQTT message, websocket payload) that needs a real device to confirm → **pytest**, `tests/api/` or `tests/mqtt/` (repo root), gated behind `--device`/`ITHO_DEVICE`.

## Native Unity test checklist

1. Match the existing suite naming: `test_native_<area>`.
2. Follow the established pattern — extract/reimplement the pure logic under test into the suite's own files rather than linking ESP32 sources (see any existing `test_native_*` suite for the shape).
3. Add a local `mocks/Arduino.h` shim only if the suite doesn't already have one and needs Arduino symbols.
4. Confirm it runs via `pio test -e native_test` from `software/NRG_itho_wifi/`.

## pytest (live hardware) checklist

1. Add to the existing `tests/api/` or `tests/mqtt/` suite matching the feature area; reuse fixtures from `conftest.py` (`device_url`, `--device` option).
2. Do not assume the test runs in CI — it doesn't (no hardware there). State in the PR/spec that it needs manual verification against a bench device.

## After

- If the test suite doesn't exist for a source file's area yet, note that gap in the spec's "Test Requirements" section rather than silently skipping coverage.
