# AGENTS.md

Canonical agent context for **emoji-picker** lives in **[`.agent/`](.agent/)**.

| Start with | Why |
|------------|-----|
| [`.agent/CONTEXT.md`](.agent/CONTEXT.md) | Goals, layout, UX contracts |
| [`.agent/RULES.md`](.agent/RULES.md) | Hard constraints |
| [`.agent/CHECKLISTS.md`](.agent/CHECKLISTS.md) | Before you finish a change |
| [`.agent/GRAPHIFY.md`](.agent/GRAPHIFY.md) | Knowledge-graph workflow |

Human docs: [`README.md`](README.md), [`docs/`](docs/).

## Essentials (always)

- `make data` regenerates `src/generated/` — never hand-edit those files.
- Multi-pick: picker stays open after emoji click.
- Paste targets the **pre-picker** X11 window (`emoji_insert_set_target`).
- C + GTK3 only for the resident UI; no Electron/Flatpak/Python UI.
- After install: `systemctl --user restart emoji-picker.service`.
- Shortcuts: Openbox **and** LXQt + `docs/SHORTCUTS.md`.

This file is a discovery stub for Cursor, Copilot, Kiro, Antigravity, Codex, and similar tools. **Edit `.agent/`**, not long prose here.
