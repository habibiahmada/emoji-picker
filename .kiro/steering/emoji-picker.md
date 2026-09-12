---
inclusion: always
---

# emoji-picker (Kiro steering)

Canonical agent context: `.agent/CONTEXT.md`, `.agent/RULES.md`, `.agent/CHECKLISTS.md`.

Hard constraints (summary):

- Never hand-edit `src/generated/`; use `make data`.
- Multi-pick stays open; paste to pre-picker X11 window.
- C/GTK3 only for the resident picker.
- Restart `emoji-picker.service` after install.
- Shortcuts: Openbox + LXQt + `docs/SHORTCUTS.md`.
- Graphify: `.agent/GRAPHIFY.md`.
