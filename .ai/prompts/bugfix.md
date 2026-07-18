# Prompt Template — Bug Fix

## Checklist before coding

1. Reproduce/confirm the failure mode first — don't guess at root cause.
2. Identify which tier the bug lives in: firmware logic (`main/`), native test gap (`test/test_native_*`), or something only visible on live hardware (`tests/api`/`tests/mqtt`).
3. For non-trivial fixes (behavior change, not just a typo/off-by-one), copy [specs/templates/bugfix-spec.md](../../specs/templates/bugfix-spec.md) into `specs/active/` — capture root cause, risk, and rollback strategy.
4. Check whether the bug touches a documented pattern ([.ai/common-patterns.md](../common-patterns.md)) — fix within the pattern, don't route around it.

## Implementation

- Prefer the smallest correct fix. This is a flash/RAM-constrained embedded target (`-Os`) — avoid adding heavyweight abstractions to fix a narrow bug.
- If the bug was in `processXxx` logic, add/extend the matching `test_native_*` case so it can't regress silently.
- If the bug was ISR-related (`main/tasks/task_cc1101.cpp`), keep the fix `IRAM_ATTR`-safe: no blocking calls, no heap allocation, no `Serial.print`.

## After

- Move the spec to `specs/completed/` once shipped.
- Note the root cause in the release notes entry ([.ai/release-process.md](../release-process.md)) if it's a user-visible fix.
