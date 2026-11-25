# ADR-0009: Dual-Tier Testing Strategy

Status: Accepted

## Context

The firmware has logic that's pure/host-testable (parsing, validation, response construction) and behavior that only manifests on real hardware talking to a real Itho unit over I2C/RF (status polling, RF timing, actual device responses). Evidence: `software/NRG_itho_wifi/test/test_native_*` suites (12 of them: api_validation, config, helpers, json, mqtt_cmd_parse, mqtt_response, queue, remote, rf, speed_parse, uuid, wifi_config) run under PlatformIO's `native_test` environment (`platform = native`, Unity framework) with local `mocks/` shims, and are wired into CI (`.github/workflows/ci.yml` `native-tests` job: `pio test -e native_test`). Separately, `tests/api/` and `tests/mqtt/` (repo root, pytest) run against a live device via `--device`/`ITHO_DEVICE` (`tests/api/conftest.py`) and are **not** in the CI workflow — no hardware is available in GitHub Actions.

## Decision

Maintain two independent test tiers: native Unity tests for pure logic (CI-gated, run on every push/PR), and live-hardware pytest integration tests for end-to-end/hardware-dependent behavior (manual, run against a bench device before release).

## Consequences

**Positive**: CI catches logic regressions on every PR without needing hardware-in-the-loop infrastructure; the pytest suite still gives comprehensive endpoint/protocol coverage for release validation, just not automatically gated.

**Negative**: hardware-dependent regressions (RF timing, actual I2C device quirks) can only be caught manually before a release, not automatically on every PR — release process ([.ai/release-process.md](../../.ai/release-process.md)) must include a manual pytest run as a checklist item, since CI won't do it.

## Alternatives Considered

- **Hardware-in-the-loop CI**: would close the gap above but requires physical device infrastructure attached to CI runners — not implemented, no evidence it was attempted.
- **Mocking hardware for the pytest suite**: would allow CI execution but the suite is explicitly designed to hit real endpoints (`DEVICE_URL`/`SPEC_URL` in `conftest.py`) for release-confidence purposes; mocking would defeat that purpose and duplicate what native tests already cover.

## Related Components

`software/NRG_itho_wifi/test/test_native_*`, `tests/api/`, `tests/mqtt/`, `.github/workflows/ci.yml`

## Related Issues

None tracked.

## AI Notes

New pure logic → add/extend a `test_native_*` suite (CI-gated). New externally-observable behavior → add a `tests/api`/`tests/mqtt` case and explicitly note in the spec that it needs manual hardware verification, since CI will not catch a regression there. See [.ai/testing-strategy.md](../../.ai/testing-strategy.md) and [.ai/prompts/test-generation.md](../../.ai/prompts/test-generation.md).
