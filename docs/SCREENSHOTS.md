# Screenshots

PNG assets for the README live in this folder:

| File | What it shows |
|------|----------------|
| `picker-hero.png` | Overview with a thin border (README hero) |
| `picker-overview.png` | Default browse grid |
| `picker-search.png` | Search for `fire` |
| `picker-search-cat.png` | Search for `cat` |
| `picker-in-context.png` | Cropped desktop with the popup (LXQt wallpaper, not IDE) |
| `desktop-with-picker.png` | Full 1920×1080 desktop capture on the real wallpaper |
| `htop-resources.png` | htop of the daemon (~31 MB RES); username redacted |

## How to refresh

```bash
# Daemon must be running; Cursor/browser are minimized automatically.
./scripts/capture-screenshots.sh
```

Composites the live picker onto the configured LXQt wallpaper
(`~/.local/share/lxqt/.../wallpapers/`) so README shots never include the IDE.

For the htop resource shot, run `htop -p $(pgrep -n -x emoji-picker)`, capture the
window, then blur the USER column and `/home/<user>` in the Command path before
committing (see README “Lightweight by design”).
