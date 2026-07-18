# Development Workflow

## Branching (observed in git history)

- `master` — stable/release branch
- `develop` — integration branch
- `feature/<name>`, `fix/<name>` — topic branches merged into `develop`
- `origin` = `arjenhiemstra/ithowifi` (the canonical repo). Contributors typically fork it and add their own fork as a second remote; there is no fixed `forked`/`upstream` remote in this repo.
- Commit prefixes in use: `feat:`, `fix:`, `docs:`, `release:`, `merge:` — match this convention for new commits

## Before starting architecturally-significant work

1. Check [specs/index.md](../specs/index.md) for an existing spec covering the area.
2. If none exists and the change is a new feature/bugfix/refactor/API change, create one from [specs/templates/](../specs/templates/) first.
3. If the change affects architecture, API surface, hardware support, MQTT topics, or the config model, also check/update the relevant [docs/adr/](../docs/adr/) ADR — see the trigger table in [docs/adr/README.md](../docs/adr/README.md).
4. After implementation, update `docs/architecture/repository-map.md` / `.ai/architecture-summary.md` if the change altered what they describe.

Full automation rule set: [.ai/ai-instructions.md](ai-instructions.md).

## Local build/test loop

See build/test commands in [AGENTS.md](../AGENTS.md) (root, canonical) — not duplicated here.

## Release flow

See [.ai/release-process.md](release-process.md).
