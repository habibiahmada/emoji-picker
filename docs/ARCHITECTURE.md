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
                                      │ xclip CLIPBOARD+PRIMARY  │
                                      │ (user presses Ctrl+V)    │
                                      └──────────────────────────┘
```

## Source layout

```
src/
  main.c                 # daemon + socket IPC
  ui/                    # GTK window, grid, dismiss
  model/                 # search, hits, history
  insert/                # clipboard paste into target app
  platform/              # X11 helpers
  generated/             # make data — never hand-edit
tests/                   # make test
```

| Module | Role |
|--------|------|
| `ui/popup.c` | Window shell, insert UX orchestration |
| `ui/picker_grid.c` | Button pool, CSS, infinite scroll |
| `ui/picker_dismiss.c` | Esc / outside / Alt+Tab dismiss poll |
| `model/emoji_model.c` | Hits list, indexes, tab/search collect |
| `model/search_query.c` | Pure tokenize / name-match helpers |
| `model/history.c` | Recent picks on disk |
| `insert/insert.c` | xclip + windowfocus Ctrl+V + clear clipboard |
| `platform/x11_util.c` | Active window / toplevel helpers |

Run unit tests: `make test`.

## IPC

Socket: `$XDG_RUNTIME_DIR/emoji-picker.sock` (fallback `/tmp`).

Messages: `toggle`, `show`, `hide` (prefix match).

Only one daemon should bind the socket. A second process without `--daemon` may
still open a one-shot window if bind fails.

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
