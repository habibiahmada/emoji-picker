# Architecture (plain English)

Think of three moving parts:

1. A **daemon** that stays running (systemd user service).
2. A **shortcut** that only sends “toggle” over a Unix socket.
3. A **popup** that copies an emoji and pastes it into the *previous* window.

```
┌─────────────────┐     datagram      ┌──────────────────────────┐
│ global shortcut │ ── emoji-picker   │ emoji-picker --daemon    │
│ (Win+.)         │    --toggle       │ GTK main loop            │
└─────────────────┘                   │  • Unix sock bind        │
                                      │  • popup window (hidden) │
                                      └────────────┬─────────────┘
                                                   │ click emoji
                                                   ▼
                                      ┌──────────────────────────┐
                                      │ xclip → CLIPBOARD        │
                                      │ xdotool windowfocus      │
                                      │   + Ctrl+V into previous │
                                      │ clear CLIPBOARD (~450ms) │
                                      └──────────────────────────┘
```

## Source layout

```
src/
  main.c                 # daemon + socket IPC
  ui/                    # GTK window, grid, dismiss
  model/                 # search, hits, history, skin tone
  insert/                # clipboard paste into target app
  platform/              # X11 + font coverage helpers
  generated/             # make data — never hand-edit
tests/                   # make test
scripts/                 # data fetch/generate, release
get.sh, install.sh       # one-line installer, installer
```

| Module | Role |
|--------|------|
| `ui/popup.c` | Window shell, insert UX orchestration |
| `ui/picker_grid.c` | Virtual grid (fixed button pool in a GtkLayout), CSS |
| `ui/picker_dismiss.c` | Esc / outside / Alt+Tab dismiss poll |
| `ui/grid_layout.c` | Pure grid geometry (cell size, pool size, slots) |
| `ui/picker_tone.c` | Skin-tone popover |
| `model/emoji_model.c` | Hits list, indexes, tab/search collect, ranking |
| `model/search_query.c` | Pure tokenize / match-scoring helpers (name vs keyword) |
| `model/history.c` | Recent picks on disk |
| `model/skin_tone.c` | Saved global skin tone, variant lookup |
| `insert/insert.c` | xclip + windowfocus Ctrl+V + clear clipboard (queued, async) |
| `platform/x11_util.c` | Active window / toplevel helpers |
| `platform/font_support.c` | Hides emoji newer than the color emoji font (fontconfig) |

Run unit tests: `make test`.

## IPC

Socket: `$XDG_RUNTIME_DIR/emoji-picker.sock` (fallback `/tmp`).

Messages: `toggle`, `show`, `hide` (prefix match). `show` currently toggles too.

Only one daemon should bind the socket. A second process without `--daemon` may
still open a one-shot window if bind fails.

## Emoji data and search

`make data` fetches Unicode `emoji-test.txt` and CLDR keyword annotations
(English + Indonesian), adds `data/keywords-extra.txt`, and
`scripts/generate_emoji_data.py` writes `src/generated/emoji_data.{h,c}`: one
entry per base emoji (name, keywords, category, version) plus a table of
skin-tone variants. Search indexes name and keyword words once, on first use.

## Insert path

On emoji click: defer past GTK grab, keep picker visible (`windowfocus` + Ctrl+V,
no `windowactivate`). Clipboard cleared ~450ms after paste. Brave freezes if paste
runs under a pointer grab or with raise/blink focus fights.

## Why GTK3

Already on Lubuntu; small binary; matches other native C tools on this host
(like `volume-osd`). GTK4 is intentionally out of scope unless requested.

## Digging deeper with Graphify

Agents can query the local code graph instead of grepping everything — see
[`.agent/GRAPHIFY.md`](../.agent/GRAPHIFY.md).
