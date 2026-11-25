# Prompt Template — Refactor

## Checklist before coding

1. Confirm the refactor doesn't change external behavior (API responses, MQTT topics/payloads, device support) — if it does, this is a feature/architecture change instead, use [.ai/prompts/feature.md](feature.md) or [.ai/prompts/architecture.md](architecture.md).
2. Copy [specs/templates/refactor-spec.md](../../specs/templates/refactor-spec.md) into `specs/active/` — state the current pattern, target pattern, and why.
3. Check [docs/adr/README.md](../../docs/adr/README.md) — if the refactor moves away from a documented baseline decision (e.g. manager singleton shape, data-table device model, flag-polling), that ADR needs superseding, not silent drift.
4. Prefer aligning code *toward* the documented patterns in [.ai/common-patterns.md](../common-patterns.md) over introducing a new one.

## Implementation

- Keep the change scoped to what the spec describes — no opportunistic unrelated cleanup in the same diff.
- Run the relevant `test_native_*` suites before and after to confirm behavior is unchanged.

## After

- Move the spec to `specs/completed/`.
- Update `docs/architecture/repository-map.md` / `.ai/architecture-summary.md` and any ADR that described the old shape.
