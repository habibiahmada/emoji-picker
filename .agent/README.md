# Agent pack — emoji-picker

This folder is the **single source of truth** for AI coding agents
(Cursor, Antigravity, Kiro, GitHub Copilot, Claude Code, and others).

Humans: start with the root [README.md](../README.md).  
Agents: read the files below before changing behavior, shortcuts, or generated data.

## What to read

| File | Who | Purpose |
|------|-----|---------|
| [CONTEXT.md](CONTEXT.md) | Agents | Goals, layout, UX contracts, desktop notes |
| [RULES.md](RULES.md) | Agents | Hard constraints (never violate without asking) |
| [CHECKLISTS.md](CHECKLISTS.md) | Agents | Pre-finish checks for insert / data / shortcuts |
| [GRAPHIFY.md](GRAPHIFY.md) | Agents + humans | Code knowledge graph (`graphify`) usage |
| [platforms/](platforms/) | Humans | How each IDE/CLI loads this pack |
| [rules/emoji-picker.md](rules/emoji-picker.md) | Antigravity (+ mirrors) | Always-on workspace rule |

## Design

- **Canonical content lives only under `.agent/`.**
- Root stubs (`AGENTS.md`, `CLAUDE.md`, `.cursor/rules/`, …) are thin pointers so each tool discovers the pack.
- Do **not** duplicate long rules in every stub — edit here, keep stubs short.

## Quick rebuild (agents)

```bash
make data && make && make install
systemctl --user daemon-reload
systemctl --user restart emoji-picker.service
emoji-picker --toggle
```
