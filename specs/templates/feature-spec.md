# Feature Spec: <name>

Status: Draft | Active | Completed
Owner:
Related ADR(s):

## Summary

One paragraph: what this feature does and why.

## Acceptance Criteria

- [ ] Criterion 1 (observable, testable)
- [ ] Criterion 2

## Architecture Impact

- Touches: (task chain / manager / device model / REST API / MQTT topics / config format / hardware abstraction / none)
- If any box above is checked, link the ADR that covers it (existing or new, per [.ai/ai-instructions.md](../../.ai/ai-instructions.md))

## Risks

- Risk 1 — likelihood/impact, mitigation
- Flash/RAM impact (this is a `-Os` constrained ESP32 target — note if the feature adds meaningful binary size or RAM usage)

## Test Requirements

- [ ] Native unit test(s): `test_native_<suite>` — what's covered
- [ ] Live-hardware test(s): `tests/api/` or `tests/mqtt/` — what's covered, or explicitly "manual verification only, no automated coverage"

## Rollback Strategy

How to revert if this ships broken — e.g. config migration reversibility, feature flag, or "safe to `git revert`, no persisted-state migration."

## Related Components

File/module list this touches.
