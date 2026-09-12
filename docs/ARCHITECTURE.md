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
                                      │ clipboard ← glyph        │
                                      │ xdotool windowactivate   │
                                      │   <saved xid> + Ctrl+V   │
                                      └──────────────────────────┘
```

## Where the emoji list comes from

1. `scripts/fetch-unicode-emoji.sh` downloads Unicode `emoji-test.txt`.
2. `scripts/generate_emoji_data.py` keeps `fully-qualified` lines, groups by
   `# group:`, and writes `src/generated/emoji_data.{c,h}`.
3. `popup.c` builds tabs (history / all / categories), search, and the button pool.

Never edit `src/generated/` by hand — run `make data`.

## IPC

Socket: `$XDG_RUNTIME_DIR/emoji-picker.sock` (fallback `/tmp`).

Messages: `toggle`, `show`, `hide` (prefix match).

Only one daemon should bind the socket. A second process without `--daemon` may
still open a one-shot window if bind fails.

## Insert path

When the popup is shown, the code remembers the focused X11 window id, then on
click: set clipboard → activate that window → send Ctrl+V → reclaim focus so you
can pick another emoji. That is why multi-pick works without closing the popup.

## Why GTK3

Already on Lubuntu; small binary; matches other native C tools on this host
(like `volume-osd`). GTK4 is intentionally out of scope unless requested.

## Digging deeper with Graphify

Agents can query the local code graph instead of grepping everything — see
[`.agent/GRAPHIFY.md`](../.agent/GRAPHIFY.md).
