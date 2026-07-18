# Refactor Spec: <name>

Status: Draft | Active | Completed
Related ADR(s):

## Current State

What pattern/structure exists today (file:line references).

## Target State

What it becomes. Must align with a documented pattern in [.ai/common-patterns.md](../../.ai/common-patterns.md) or a new ADR — not an ad-hoc restructure.

## Why

What problem the current shape causes (not "cleaner code" alone — be concrete: testability, coupling, size, a recurring bug class, etc).

## Acceptance Criteria

- [ ] External behavior unchanged (list what "external" means here: API responses / MQTT payloads / device support / config file format)
- [ ] All existing `test_native_*` suites touched still pass unmodified in assertions (only internal wiring changes)

## Architecture Impact

- Touches: (task chain / manager / device model / REST API / MQTT topics / config format / hardware abstraction / none)
- If this moves away from a baseline ADR's documented decision, that ADR must be marked Superseded, not silently left stale.

## Risks

- What could regress silently (embedded target — no easy rollback via user-facing revert; consider OTA implications)

## Test Requirements

- [ ] Before/after `test_native_*` run showing no behavior change
- [ ] Any new internal seams get their own native test coverage

## Rollback Strategy

Is this a same-PR-revertable refactor, or does it require sequencing (e.g. config format migration)?

## Related Components

File/module list this touches.
