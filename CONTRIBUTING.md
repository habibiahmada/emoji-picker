# Contributing

Thanks for considering a contribution to **emoji-picker**.

## Ground rules

- Keep the app **lightweight**: C + GTK3 only for the resident UI. No Electron,
  Flatpak runtime, or Python UI for the daemon.
- Do **not** hand-edit `src/generated/` — regenerate with `make data`.
- Preserve UX contracts in `AGENTS.md` / `README.md` unless the change is
  intentional and documented (multi-pick stays open, insert targets the
  pre-picker X11 window, etc.).
- Repo docs stay in **English**; chat can be any language.

## Development setup

```bash
sudo apt install build-essential pkg-config libgtk-3-dev xdotool \
  fonts-noto-color-emoji curl python3

git clone https://github.com/habibiahmada/emoji-picker.git
cd emoji-picker
make data && make
./emoji-picker --daemon &
./emoji-picker --toggle
```

After `make install`:

```bash
systemctl --user daemon-reload
systemctl --user restart emoji-picker.service
```

Scripts must use Unix LF line endings.

## Pull requests

1. Fork and create a branch from `main`.
2. Keep changes focused — one concern per PR when practical.
3. Update docs (`README.md`, `docs/SHORTCUTS.md`, …) if behavior or shortcuts
   change.
4. Open a PR with a short summary and test notes (DE, shortcut, insert target).

### Checklist (insert / paste path)

- [ ] Still targets the previous X11 window id
- [ ] Works while the picker stays visible
- [ ] Does not require hiding the popup to insert

## Issues

- Bug reports: use the Bug report template.
- Features: use the Feature request template.
- Security: see [SECURITY.md](SECURITY.md) — **no public issues**.

## Code of conduct

Be respectful. See [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md).
