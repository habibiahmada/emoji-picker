# Contributing

Thanks for considering a contribution to **emoji-picker**.

## Ground rules

- Keep the app **lightweight**: C + GTK3 only for the resident UI. No Electron,
  Flatpak runtime, or Python UI for the daemon.
- Do **not** hand-edit `src/generated/` — regenerate with `make data`.
- Preserve the UX contracts in [`.agent/CONTEXT.md`](.agent/CONTEXT.md) unless
  the change is intentional and documented (multi-pick stays open, insert
  targets the pre-picker X11 window, one saved skin tone, etc.).
- Repo docs stay in **English**; chat can be any language.

## Development setup

```bash
sudo apt install build-essential pkg-config libgtk-3-dev libx11-dev \
  libfontconfig-dev xdotool xclip fonts-noto-color-emoji curl python3

git clone https://github.com/habibiahmada/emoji-picker.git
cd emoji-picker
make && make test     # src/generated/ is committed; `make data` refreshes it
./emoji-picker --daemon &
./emoji-picker --toggle
```

`make install` runs `./install.sh`. After it:

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
4. Run `make test`; CI runs it on every push and pull request.
5. Open a PR with a short summary and test notes (DE, shortcut, insert target).

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
