# ADR-0001: AI Documentation Framework

Status: Accepted

## Context

The repository had no AI-agent-facing documentation: no `CLAUDE.md`, no `.ai/`, no ADRs, no specs, no Cursor/Copilot instructions. Architecture knowledge (task chain, manager pattern, data-table device model, API logic/routing split) existed only as tribal knowledge in code — every AI coding session re-derived it from scratch at high token cost, with no mechanism to prevent architectural drift or duplicate documentation.

## Decision

Introduce a structured, token-efficient documentation layer:
- `docs/architecture/repository-map.md` — canonical architecture reference with diagrams
- `.ai/` — condensed, agent-facing context files (project overview, architecture summary, coding standards, glossary, patterns, workflow, testing, release, security, meta-instructions, prompt templates, context map)
- `specs/` — spec-first workflow for features/bugfixes/refactors/API changes, with lifecycle folders and templates
- `docs/adr/` — Architecture Decision Records for durable decisions, with a trigger table linking change types to required ADRs
- `CLAUDE.md` (root) — Claude Code's entry-point context file
- `.cursor/rules/*.mdc` — Cursor AI enforcement rules
- `.github/copilot-instructions.md` — GitHub Copilot repo instructions

All content is sourced from an actual repository scan (tasks, managers, api, ithodevice, cc1101, config, tests, CI) — no invented architecture.

## Consequences

**Positive**: agents load condensed context instead of re-deriving it; architectural patterns (manager singletons, data-table devices, logic/routing split, flag-polling) are documented once and linked, not restated; specs/ADRs give a checkpoint before architecturally significant changes land.

**Negative**: documentation now requires upkeep — stale docs are worse than no docs. Mitigated by the update-triggers documented in [.ai/ai-instructions.md](../../.ai/ai-instructions.md) and this ADR index's trigger table.

## Alternatives Considered

- **Single large README/architecture doc**: rejected — doesn't scale token-wise, and gives AI agents no structured signal about *when* to update it or write an ADR.
- **Wiki-based docs (external to repo)**: rejected — drifts from code faster since it's not co-reviewed in PRs.
- **No ADRs, specs only**: rejected — specs capture per-change intent but not durable cross-cutting decisions (e.g. "why flag-polling instead of queues") that outlive any single spec.

## Related Components

`docs/`, `.ai/`, `specs/`, `CLAUDE.md`, `.cursor/rules/`, `.github/copilot-instructions.md`

## Related Issues

None — internal documentation initiative.

## AI Notes

This ADR is self-referential: it documents the framework you are reading. Follow [.ai/ai-instructions.md](../../.ai/ai-instructions.md) for when to consult/update the rest of this framework. Do not create a second competing documentation structure (e.g. a new top-level `architecture/` or `guides/` folder) — extend what exists here.
