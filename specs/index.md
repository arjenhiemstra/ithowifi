# Specs Index

Specs capture *intent* for a change before code exists. They live alongside the code they describe, not as a substitute for it.

## Lifecycle

```
specs/future/     → not yet started, may change before work begins
specs/active/      → currently being implemented
specs/completed/   → shipped; kept as historical record, linked from release notes/ADRs where relevant
```

Move a spec between folders (don't copy) as its state changes.

## When a spec is required

Every new feature, and every non-trivial bugfix or refactor, gets a spec **before** implementation. Use the matching template from [specs/templates/](templates/):

| Change type | Template |
|---|---|
| New feature | [feature-spec.md](templates/feature-spec.md) |
| Bug fix (non-trivial) | [bugfix-spec.md](templates/bugfix-spec.md) |
| New/changed REST or MQTT API surface | [api-spec.md](templates/api-spec.md) |
| Refactor | [refactor-spec.md](templates/refactor-spec.md) |

Every spec must define: **acceptance criteria, risks, architecture impact, test requirements, rollback strategy.**

## Spec ↔ ADR relationship

A spec describes *what* is being built and its immediate risk/test/rollback plan. An ADR records a *durable architectural decision* (task structure, manager shape, API design pattern, config format, hardware abstraction). Not every spec needs an ADR — only specs whose "Architecture Impact" section is non-trivial do. See the trigger table in [docs/adr/README.md](../docs/adr/README.md).

## Active specs

_(none yet — add entries here as specs are created in `specs/active/`)_

## Related

[.ai/ai-instructions.md](../.ai/ai-instructions.md) · [.ai/development-workflow.md](../.ai/development-workflow.md) · [docs/adr/README.md](../docs/adr/README.md)
