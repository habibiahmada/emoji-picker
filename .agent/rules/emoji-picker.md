---
description: emoji-picker always-on workspace rules
alwaysApply: true
globs:
  - "**/*"
---

# emoji-picker

Read `.agent/CONTEXT.md` and `.agent/RULES.md` before changing behavior.

- Regenerate emoji tables with `make data`; never hand-edit `src/generated/`.
- Keep insert targeting the pre-picker X11 window (`emoji_insert_set_target`).
- Keep multi-pick: picker stays visible on emoji click; close only Esc / outside / focus-out.
- Prefer C/GTK3; do not introduce Electron, Flatpak, or Python UI.
- After install changes: `systemctl --user restart emoji-picker.service`.
- Shortcut changes: Openbox + LXQt (or GUI) and `docs/SHORTCUTS.md`.
- Scripts: Unix LF only.
- Canonical agent pack: `.agent/` (not duplicated per IDE).
- Use Graphify (`graphify query` / see `.agent/GRAPHIFY.md`) for structural questions when available.
