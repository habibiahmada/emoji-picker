# emoji-picker — agent context

Lightweight Unicode emoji popup for Linux **X11** desktops (especially Lubuntu/LXQt,
~8 GB RAM). Prefer **C + GTK3** over Python/Flatpak/Electron for anything that stays resident.

Human overview: `README.md`. Architecture sketch: `docs/ARCHITECTURE.md`.

## Goal

Ship a fast, up-to-date Unicode emoji popup: daemon stays warm, **Win+.** (`Meta+period`)
toggles instantly, multi-pick without closing, paste into the **pre-picker** window.

## Non-goals

- Wayland-first / portal paste (unless the user asks)
- Per-emoji skin-tone memory (one saved tone applies to every toned emoji)
- Packaging for every distro before the local install works
- Re-adding Emote Flatpak or `gnome-characters` as the primary picker

## Source of truth

| Concern | Location |
|---------|----------|
| Emoji glyphs + names | `data/emoji-test.txt` → `scripts/generate_emoji_data.py` → `src/generated/*` |
| Search keywords | CLDR `data/cldr/*.xml` (fetched, en + id) + `data/keywords-extra.txt` (hand-kept) → generator |
| UI shell | `src/ui/popup.c` |
| Grid / CSS | `src/ui/picker_grid.c` |
| Dismiss poll | `src/ui/picker_dismiss.c` |
| Search + hits | `src/model/emoji_model.c` + `src/model/search_query.c` |
| X11 helpers | `src/platform/x11_util.c` |
| Recent history | `src/model/history.c` → `~/.local/share/emoji-picker/history` |
| Skin tone | `src/model/skin_tone.c` → `~/.local/share/emoji-picker/skin-tone`; popup `src/ui/picker_tone.c` |
| Font coverage filter | `src/platform/font_support.c` (fontconfig) |
| Insert into apps | `src/insert/insert.c` |
| Daemon / IPC | `src/main.c` |
| Unit tests | `tests/` — `make test` |
| User unit | `pack/emoji-picker.service` |
| Installed binary | `~/.local/bin/emoji-picker` |
| Agent rules / context | **`.agent/`** |

**Never hand-edit** `src/generated/emoji_data.{h,c}`. Regenerate with `make data`.

## Commands agents should use

```bash
cd ~/projects/emoji-picker   # adjust if relocated
./install.sh                 # build if needed, install, shortcut setup
# Or via make:
make test && make install
# smoke:
emoji-picker --toggle
```

End users install with `get.sh` (one line, no clone): it downloads the latest
release tarball, verifies the `.sha256` and runs the bundled `install.sh`. Keep
`get.sh` POSIX `sh` (it is piped to `sh`) and keep its tarball/checksum names in
sync with `scripts/release.sh`; `tests/test_install.sh` covers it offline.

Scripts must be LF (not CRLF). If `env: 'bash\r'` appears:

```bash
sed -i 's/\r$//' scripts/*
```

## UX contracts (do not break without asking)

1. **Win+.** (Meta+period) toggles the picker — primary user binding.
2. Picker **stays visible** on emoji click (multi-pick). Close only when the user
   leaves the picker (see 4).
3. On emoji click: defer until GTK grab is gone; picker stays mapped with
   keep_above (no blink). Refuse accept_focus briefly; `windowfocus` + Ctrl+V
   (never windowactivate / hide). Clear CLIPBOARD ~450ms after paste.
4. Esc / click-outside / typing in the target app / Alt+Tab to another app hides
   the window (dismiss poll — not focus-out alone). Process remains as daemon.
5. Skip taskbar / utility window; place near pointer.
6. Search matches Unicode English names (stored lowercase) and per-emoji keywords
   (CLDR en + id, plus `data/keywords-extra.txt`). Every query word must match;
   order: whole name > name word > keyword > name-word prefix > other.
7. History tab shows recent picks when available; do not remove casually.
   It shows the exact glyph picked (tone included) and inserts it as-is.
8. Skin tone: grid shows the base (yellow) glyph until a tone is chosen. The
   first click on a toned emoji opens the tone popup; a uniform pick is saved
   and applied to **every** toned emoji. After that, click inserts directly;
   right-click reopens the popup. Mixed two-person picks insert but do not
   change the saved tone.

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

Emoji picker is a **separate** project under `~/projects/emoji-picker`.

## Style

- C11-ish, GTK3 only (no GTK4 migration unless requested)
- No new heavy deps
- Makefile install prefix stays `~/.local`
- Chat may be Indonesian; **repo docs stay English** unless the user asks otherwise
- Agent pack path: `.agent/` — keep platform stubs thin

## Knowledge graph

Prefer querying `graphify-out/graph.json` (see [GRAPHIFY.md](GRAPHIFY.md)) before broad greps
when exploring call/import relationships.
