# emoji-picker — agent context

Lightweight Unicode emoji popup for Linux **X11** desktops (especially Lubuntu/LXQt,
~8 GB RAM). Prefer **C + GTK3** over Python/Flatpak/Electron for anything that stays resident.

Human overview: `README.md`. Architecture sketch: `docs/ARCHITECTURE.md`.

## Goal

Ship a fast, up-to-date Unicode emoji popup: daemon stays warm, **Win+.** (`Meta+period`)
toggles instantly, multi-pick without closing, paste into the **pre-picker** window.

## Non-goals

- Wayland-first / portal paste (unless the user asks)
- Heavy skin-tone UI beyond what Unicode data already includes
- Packaging for every distro before the local install works
- Re-adding Emote Flatpak or `gnome-characters` as the primary picker

## Source of truth

| Concern | Location |
|---------|----------|
| Emoji glyphs + names | `data/emoji-test.txt` → `scripts/generate_emoji_data.py` → `src/generated/*` |
| UI shell | `src/ui/popup.c` |
| Grid / CSS | `src/ui/picker_grid.c` |
| Dismiss poll | `src/ui/picker_dismiss.c` |
| Search + hits | `src/model/emoji_model.c` + `src/model/search_query.c` |
| X11 helpers | `src/platform/x11_util.c` |
| Recent history | `src/model/history.c` → `~/.local/share/emoji-picker/history` |
| Insert into apps | `src/insert/insert.c` |
| Daemon / IPC | `src/main.c` |
| Unit tests | `tests/` — `make test` |
| User unit | `pack/emoji-picker.service` |
| Installed binary | `~/.local/bin/emoji-picker` |
| Agent rules / context | **`.agent/`** |

**Never hand-edit** `src/generated/emoji_data.{h,c}`. Regenerate with `make data`.

## Commands agents should use

```bash
cd /home/habibiahmada/Projects/emoji-picker   # adjust if relocated
./install.sh                                  # one-step install & shortcut setup
# Or via make:
make test && make install
# smoke:
emoji-picker --toggle
```

Scripts must be LF (not CRLF). If `env: 'bash\r'` appears:

```bash
sed -i 's/\r$//' scripts/*
```

## UX contracts (do not break without asking)

1. **Win+.** (Meta+period) toggles the picker — primary user binding.
2. Picker **stays visible** on emoji click (multi-pick). Close only via Esc /
   click-outside / focus-out when the user leaves the picker.
3. On emoji click: defer until GTK grab is gone; picker stays mapped with
   keep_above (no blink). Refuse accept_focus briefly; `windowfocus` + Ctrl+V
   (never windowactivate / hide). Clear CLIPBOARD ~450ms after paste.
4. Esc / click-outside / typing in the target app / Alt+Tab to another app hides
   the window (dismiss poll — not focus-out alone). Process remains as daemon.
5. Skip taskbar / utility window; place near pointer.
6. Search matches Unicode English names (stored lowercase).
7. History tab shows recent picks when available; do not remove casually.

## Desktop integration (this machine)

- DE: **LXQt** + **Openbox** on Lubuntu.
- Bind **both** LXQt global keys and Openbox when adding Super shortcuts.
- Prefer documenting GUI paths (`LXQt Settings → Shortcut Keys`) when file edits and the live GUI diverge.
- Display for user systemd services often needs a drop-in:
  `~/.config/systemd/user/emoji-picker.service.d/display.conf` with `DISPLAY` and `XAUTHORITY`
  (same pattern as `volume-osd`).

## Related user tools (do not remove casually)

- `~/.local/bin/volume-osd` + `volume-osd.service`
- `~/.local/bin/default-sink-volume.sh`
- Soft left speaker: WirePlumber + `fix-left-speaker` user service

Emoji picker is a **separate** project under `~/Projects/emoji-picker`.

## Style

- C11-ish, GTK3 only (no GTK4 migration unless requested)
- No new heavy deps
- Makefile install prefix stays `~/.local`
- Chat may be Indonesian; **repo docs stay English** unless the user asks otherwise
- Agent pack path: `.agent/` — keep platform stubs thin

## Knowledge graph

Prefer querying `graphify-out/graph.json` (see [GRAPHIFY.md](GRAPHIFY.md)) before broad greps
when exploring call/import relationships.
