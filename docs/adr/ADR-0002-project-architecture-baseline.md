# ADR-0002: Project Architecture Baseline

Status: Accepted

## Context

Before this documentation framework existed, ithowifi had already accumulated a consistent set of architectural decisions through incremental development (not a single up-front design): a linear FreeRTOS task chain, global manager singletons, a data-table device model, and a logic/routing split in the API layer. These were never written down. Future changes need a documented baseline to diff against, or drift is invisible until it causes a bug or an inconsistent AI-generated change.

## Decision

Record the pre-existing baseline architecture as accepted, snapshotted from the actual codebase at the time this framework was introduced:

1. Boot/runtime is a **linear task chain** (`TaskInit → TaskConfigAndLog → TaskSysControl → TaskCC1101 → TaskMQTT → TaskWeb`), single core, each task starting the next — detailed in [ADR-0004](ADR-0004-freertos-linear-task-chain.md).
2. Hardware/network/RF/I2C access goes through **global manager singleton instances** (`main/managers/*`) — detailed in [ADR-0008](ADR-0008-hardware-abstraction-managers.md).
3. Device-specific behavior is a **data table** (`ithoDeviceType ithoDevices[]`), not class inheritance.
4. The REST API separates **pure logic (`processXxx`) from framework routing (`handleXxx`)** — detailed in [ADR-0006](ADR-0006-rest-api-v2-logic-routing-split.md).
5. Config persists to **LittleFS JSON + NVS backup** — detailed in [ADR-0007](ADR-0007-config-persistence-littlefs-nvs.md).
6. Testing is **two-tier**: native host-side Unity tests for pure logic, live-hardware pytest for integration — detailed in [ADR-0009](ADR-0009-dual-tier-testing-strategy.md).

This ADR is the baseline pointer; ADRs 0003–0009 carry the per-topic detail and evidence.

## Consequences

**Positive**: gives every subsequent architectural change (and every AI agent) a documented "current state" to compare against instead of inferring it from source each time.

**Negative**: none of these were deliberately chosen against alternatives at the time — they emerged from iterative development. This ADR documents them as accepted baseline, not as a claim they were the optimal choice from a blank slate.

## Alternatives Considered

Not applicable — this ADR documents existing, already-implemented decisions rather than proposing a new one. See ADRs 0003–0009 for topic-specific alternatives considered where relevant.

## Related Components

`main/tasks/`, `main/managers/`, `main/ithodevice/`, `main/api/`, `main/config/`, `software/NRG_itho_wifi/test/`, `tests/`

## Related Issues

None.

## AI Notes

Treat items 1–6 above as the default architecture to extend, not to redesign opportunistically. Any change that would alter one of these six baseline facts is architecturally significant — follow the spec+ADR workflow in [.ai/ai-instructions.md](../../.ai/ai-instructions.md) rather than changing it inline.
