# Architecture Decision Records

ADRs record durable architectural decisions — not implementation details. Use the [template](templates/adr-template.md). Number sequentially, never reuse a number, never delete a file — mark it `Superseded` and link forward instead.

## Index

| ADR | Title | Status |
|---|---|---|
| [0001](ADR-0001-ai-documentation-framework.md) | AI documentation framework | Accepted |
| [0002](ADR-0002-project-architecture-baseline.md) | Project architecture baseline | Accepted |
| [0003](ADR-0003-platformio-espidf-arduino.md) | PlatformIO + dual Arduino/ESP-IDF framework | Accepted |
| [0004](ADR-0004-freertos-linear-task-chain.md) | FreeRTOS linear task chain, flag-polling | Accepted |
| [0005](ADR-0005-mqtt-integration-design.md) | MQTT integration design | Accepted |
| [0006](ADR-0006-rest-api-v2-logic-routing-split.md) | REST API v2 logic/routing split | Accepted |
| [0007](ADR-0007-config-persistence-littlefs-nvs.md) | Config persistence: LittleFS + NVS backup | Accepted |
| [0008](ADR-0008-hardware-abstraction-managers.md) | Hardware abstraction via manager singletons | Accepted |
| [0009](ADR-0009-dual-tier-testing-strategy.md) | Dual-tier testing strategy | Accepted |

## When an ADR is required

Trigger table — if a change matches a row, check the linked ADR before implementing, and update it (or add a new one, marking the old one Superseded) if the decision itself changes:

| Change area | Relevant ADR(s) |
|---|---|
| Task structure, RTOS primitives, inter-task signaling | [0004](ADR-0004-freertos-linear-task-chain.md) |
| MQTT topics/payloads | [0005](ADR-0005-mqtt-integration-design.md) |
| REST API structure/routing | [0006](ADR-0006-rest-api-v2-logic-routing-split.md) |
| Config file format / storage backend | [0007](ADR-0007-config-persistence-littlefs-nvs.md) |
| New hardware peripheral / manager | [0008](ADR-0008-hardware-abstraction-managers.md) |
| Test infrastructure / CI test gating | [0009](ADR-0009-dual-tier-testing-strategy.md) |
| Toolchain/platform/framework | [0003](ADR-0003-platformio-espidf-arduino.md) |
| Anything not covered above but structurally significant | [0002](ADR-0002-project-architecture-baseline.md) as baseline reference; propose a new ADR |

## Workflow

1. Check this index for an existing ADR covering the area.
2. If none exists and the change is architecturally significant (see [.ai/ai-instructions.md](../../.ai/ai-instructions.md)), copy [templates/adr-template.md](templates/adr-template.md), fill it in, status `Proposed`.
3. Link the ADR from the relevant spec ([specs/index.md](../../specs/index.md)) and from `docs/architecture/repository-map.md` if it changes the map.
4. On acceptance, set status `Accepted` and add it to the index table above.
5. If a later ADR replaces this decision, set this one's status to `Superseded (by ADR-YYYY)` — never delete.

## Related

[specs/index.md](../../specs/index.md) · [.ai/ai-instructions.md](../../.ai/ai-instructions.md) · [docs/architecture/repository-map.md](../architecture/repository-map.md)
