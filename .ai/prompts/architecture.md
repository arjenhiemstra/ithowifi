# Prompt Template — Architecture Change

Use for changes to: task chain/RTOS structure, manager responsibilities, device data-model shape, REST API surface, MQTT topic structure, config persistence format, or hardware abstraction.

## Checklist before coding

1. Read [docs/architecture/repository-map.md](../../docs/architecture/repository-map.md) in full for the area being changed.
2. Read every existing ADR in [docs/adr/](../../docs/adr/) that touches this area — determine whether you're superseding one.
3. Write a spec from [specs/templates/architecture change → use feature-spec.md or refactor-spec.md as fits](../../specs/templates/), covering architecture impact explicitly.
4. Draft a new ADR from [docs/adr/templates/adr-template.md](../../docs/adr/templates/adr-template.md) — Context, Decision, Consequences, Alternatives Considered, Related Components, Related Issues, AI Notes. If it replaces an existing ADR, mark that ADR "Superseded" and link forward.
5. Get the spec + ADR reviewed/agreed before implementing — this is the one category where doc-first is a hard requirement, not a nice-to-have.

## Implementation

- Implement to match what the ADR/spec describe. If reality diverges during implementation, update the docs before merging, not after.

## After

- Update `docs/architecture/repository-map.md`, `.ai/architecture-summary.md`, and `.ai/context-map.md` if they now describe stale behavior.
- Move the spec to `specs/completed/`.
- Set the ADR status to `Accepted`.
