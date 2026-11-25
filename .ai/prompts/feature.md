# Prompt Template — New Feature

Use for any new capability (endpoint, MQTT topic, device support, UI control).

## Checklist before coding

1. Read [.ai/architecture-summary.md](../architecture-summary.md) and [.ai/common-patterns.md](../common-patterns.md).
2. Check [specs/index.md](../../specs/index.md) for an existing spec; if none, copy [specs/templates/feature-spec.md](../../specs/templates/feature-spec.md) into `specs/active/` and fill it in (acceptance criteria, risks, architecture impact, test requirements, rollback strategy).
3. If the feature changes API surface, MQTT topics, device model, or config shape — this is architecturally significant, see [.ai/ai-instructions.md](../ai-instructions.md) trigger table; draft/update an ADR too.
4. Identify which existing pattern applies: manager singleton, data-table device row, or logic/routing split ([.ai/common-patterns.md](../common-patterns.md)) — reuse it, don't invent a new shape.

## Implementation

- Business logic in `processXxx` (pure, testable) if it's API-related; framework binding stays a thin `handleXxx`.
- Add/extend a `test_native_*` suite for new pure logic; add a `tests/api`/`tests/mqtt` case for anything externally observable (run manually against hardware).

## After

- Move the spec to `specs/completed/` once shipped.
- Update `docs/architecture/repository-map.md` / `.ai/architecture-summary.md` if the map is now stale.
