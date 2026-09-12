# Hard rules — emoji-picker

Violate only if the user explicitly asks.

1. Regenerate emoji tables with `make data`; **never** hand-edit `src/generated/`.
2. Keep insert targeting the **pre-picker** X11 window (`emoji_insert_set_target`).
3. Keep multi-pick: **do not** auto-hide on emoji click unless the user requests it.
4. Prefer C/GTK3; do **not** introduce Electron, Flatpak, or a Python UI for the resident picker.
5. After install changes: `systemctl --user restart emoji-picker.service`.
6. Shortcut changes: update Openbox **and** LXQt (or instruct the GUI), plus `docs/SHORTCUTS.md`.
7. Scripts must use Unix **LF** line endings.
8. Canonical agent context lives in **`.agent/`**. Update that pack first; keep root/platform stubs as short pointers.
9. Do not rebind Win+. to Flatpak Emote / `gnome-characters` unless asked.
10. Before structural exploration, use Graphify when available (`graphify query` / `graphify affected`) — see [GRAPHIFY.md](GRAPHIFY.md).
