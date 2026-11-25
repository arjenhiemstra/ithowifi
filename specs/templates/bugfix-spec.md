# Bugfix Spec: <name>

Status: Draft | Active | Completed
Related issue:

## Symptom

What's observed, how to reproduce.

## Root Cause

What's actually wrong (file:line where possible). Don't skip this — patching symptoms without a confirmed root cause is not acceptable here.

## Acceptance Criteria

- [ ] Reproduction case no longer fails
- [ ] No regression in adjacent behavior (list what was checked)

## Architecture Impact

- Touches: (task chain / manager / device model / REST API / MQTT topics / config format / hardware abstraction / none — usually none for a bugfix)
- If non-trivial, link/create an ADR per [.ai/ai-instructions.md](../../.ai/ai-instructions.md)

## Risks

- What could this fix break elsewhere (e.g. ISR timing, shared flags, other devices in the `ithoDevices[]` table)

## Test Requirements

- [ ] Native unit test reproducing the bug, now passing
- [ ] Live-hardware verification if the bug was hardware-observable only

## Rollback Strategy

How to revert if the fix itself causes a regression.

## Related Components

File/module list this touches.
