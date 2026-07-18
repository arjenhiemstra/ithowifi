# ADR-0008: Hardware Abstraction via Manager Singletons

Status: Accepted

## Context

The firmware runs on multiple hardware revisions (CVE add-on, non-CVE add-on, with/without CC1101, with/without I2C sniffer capability) and needs single, consistent access points to hardware/network/RF/I2C state across many tasks. Evidence: `main/managers/HardwareManager.h:11,67` (`hardwareManager` — pin table, revision `detect()`, sniffer-capability flag), `I2CManager.h:18,81` (`i2cManager` — pins, mutex-guarded command queue), `NetworkManager.h:7,21` (`networkManager`), `RFManager.h:6,20` (`rfManager` — owns the CC1101 radio + packet). Each is declared `extern <Class> <instance>;` and defined once — a global instance, not a class agents instantiate per-use.

## Decision

Represent each hardware/cross-cutting subsystem as a single global manager instance under `main/managers/`, accessed directly by name from any task/handler that needs it, rather than passing references/pointers around or using dependency injection.

## Consequences

**Positive**: simple access from any file without plumbing references through call chains (relevant given the flag-polling task model in [ADR-0004](ADR-0004-freertos-linear-task-chain.md)); one clear owner per subsystem's state; matches embedded-firmware conventions where there's exactly one of each hardware peripheral.

**Negative**: global mutable state requires discipline around thread-safety (see `I2CManager::queueMutex`) since multiple tasks can touch a manager; harder to substitute a manager for testing than an injected dependency — evidenced by `test_native_*` suites extracting logic rather than mocking managers directly.

## Alternatives Considered

- **Dependency injection / passed-in references**: not used anywhere in `main/managers/`; would add boilerplate for a single-instance-per-hardware-peripheral reality with limited benefit given there's exactly one ESP32, one CC1101, one I2C bus.
- **Per-task-owned state (no shared manager)**: not viable since multiple tasks need the same hardware (e.g. both `TaskCC1101`'s ISR and other tasks need `rfManager`).

## Related Components

`main/managers/HardwareManager.h`, `main/managers/I2CManager.h`, `main/managers/NetworkManager.h`, `main/managers/RFManager.h`, `main/managers/SecureWebCommLite.h`

## Related Issues

None tracked.

## AI Notes

Add a new hardware/cross-cutting subsystem as a new `main/managers/<Name>Manager.h` following the existing `extern <Class> <instance>;` shape — see [.ai/common-patterns.md](../../.ai/common-patterns.md) #1. Don't construct a local instance of an existing manager class; use the global instance.
