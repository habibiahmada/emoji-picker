# emoji-picker

![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-Linux%20X11-green.svg)
![Built with](https://img.shields.io/badge/built%20with-C%20%2B%20GTK3-orange.svg)

**Win+.** → pick emoji → inserts into the app you were already using.

A tiny **C + GTK3** Unicode emoji popup for Linux **X11** (Lubuntu/LXQt and friends).
No Electron. No Flatpak tax. A warm daemon so the shortcut feels instant.

![emoji-picker popup](docs/screenshots/picker-hero.png)

![Browse all emoji](docs/screenshots/picker-overview.png)   ![Search for cat](docs/screenshots/picker-search-cat.png)   ![Search results](docs/screenshots/picker-search.png)

## Why this exists

On a low-RAM desktop, “just install another Flatpak emoji app” is the wrong trade.
This project stays small, uses system GTK, and inserts into the **window that was
focused before** the picker opened — so you can keep multi-picking without fighting focus.

## Features

- Full Unicode set from official `emoji-test.txt` (refresh anytime)
- Search by English name (`smile`, `fire`, `cat`, …)
- Category tabs + recent history
- Popup near the pointer; skip taskbar; Esc / click-outside to dismiss
- Stays open for multi-pick (closes on Esc / outside / typing / Alt+Tab)
- Inserts via clipboard into the pre-picker window; clears clipboard after paste
- Daemon + Unix socket: `emoji-picker --toggle` from a global shortcut



## Lightweight by design

Built for low-RAM X11 desktops — not another Electron/Flatpak resident.


| Metric                                | Value                    |
| ------------------------------------- | ------------------------ |
| Binary on disk                        | **~225 KB**              |
| Emoji table (generated)               | **~80 KB** · 1914 glyphs |
| Daemon RSS (idle)                     | **~30 MB**               |
| Daemon RSS (popup open)               | **~45 MB**               |
| PSS idle / open (fairer share of GTK) | **~12 / ~18 MB**         |


Compared to the earlier naïve UI (~157 MB RSS from thousands of live widgets), the
pool + pagination design keeps the warm daemon small enough to leave running.

![htop showing emoji-picker ~31 MB RES](docs/screenshots/htop-resources.png)

## Screenshots


| Browse                                            | Search                                                |
| ------------------------------------------------- | ----------------------------------------------------- |
| ![overview](docs/screenshots/picker-overview.png) | ![cat search](docs/screenshots/picker-search-cat.png) |


Picker on the desktop (LXQt):

![desktop](docs/screenshots/picker-in-context.png)

## Quick start

### One-step installation (recommended)

Clone and run the installer:

```bash
git clone https://github.com/habibiahmada/emoji-picker.git
cd emoji-picker
./install.sh
```

**That's it! 🎉** Press **Win + .** (`Super+Period`) anywhere to toggle the emoji picker.

#### What `./install.sh` does automatically:
1. **Checks dependencies** (runtime & build) and detects your package manager if anything is missing.
2. **Builds the binary** if not already compiled (`make data && make`).
3. **Installs binary** to `~/.local/bin/emoji-picker`.
4. **Configures & starts systemd user service** with proper X11 `DISPLAY` and `XAUTHORITY` drop-in configuration.
5. **Configures global shortcut (Win+.)** automatically for your desktop environment:
   - **LXQt**: registers `Meta+period` in `globalkeyshortcuts.conf` and reloads `lxqt-globalkeysd`.
   - **Openbox**: adds `<keybind key="W-period">` to `rc.xml` and triggers `openbox --reconfigure`.
   - **GNOME**: adds custom keybinding via `gsettings`.
   - **XFCE**: configures `<Super>period` via `xfconf-query`.
   - **i3 / Sway**: appends shortcut to config and reloads.
6. **Verifies installation** and ensures the warm daemon is active.

---

### Pre-built binary from Releases

If you downloaded a release from [GitHub Releases](https://github.com/habibiahmada/emoji-picker/releases):

```bash
tar -xzf emoji-picker-*-linux-x86_64.tar.gz
cd emoji-picker-*-linux-x86_64
./install.sh
```

<details>
<summary>Manual build & advanced options</summary>

**Installer options:**
```bash
./install.sh --help           # Show all options
./install.sh --dry-run        # Preview actions without modifying anything
./install.sh --check          # Check requirements and dependencies only
./install.sh --no-shortcut    # Skip desktop shortcut registration
./install.sh --no-service     # Skip systemd user service setup
./install.sh --uninstall      # Clean uninstallation
./install.sh --prefix=/opt    # Custom install prefix (default: ~/.local)
```

**Dependencies:**
- Debian / Ubuntu / Lubuntu:
  ```bash
  sudo apt install build-essential pkg-config libgtk-3-dev libx11-dev xdotool fonts-noto-color-emoji xclip
  ```
- Fedora:
  ```bash
  sudo dnf install gcc make pkg-config gtk3-devel libX11-devel xdotool google-noto-color-emoji-fonts xclip
  ```
- Arch Linux / Manjaro:
  ```bash
  sudo pacman -S --needed base-devel gtk3 libx11 xdotool noto-fonts-emoji xclip
  ```

**Manual compilation with Makefile:**
```bash
make data                     # Download Unicode & generate data tables
make                          # Compile binary
make install                  # Runs ./install.sh
```

See [docs/SHORTCUTS.md](docs/SHORTCUTS.md) for manual shortcut configuration details.
</details>

Maintainer note: ship a release with `./scripts/release.sh 0.3.0 --publish` (or push a `v*` tag).

## Updating

Check for updates or upgrade to the newest release anytime:

```bash
# Check if an update is available:
./install.sh --check-update

# Upgrade to the latest version:
./install.sh --update
# (or 'make update')
```

This automatically checks Git origin (or GitHub releases for standalone installs), refreshes Unicode data, recompiles, re-installs, and restarts the background daemon.

## Usage

| Command                          | Meaning                                   |
| -------------------------------- | ----------------------------------------- |
| `emoji-picker`                   | Start (shows popup; binds socket if free) |
| `emoji-picker --daemon`          | Start hidden (systemd)                    |
| `emoji-picker --toggle`          | Show/hide (shortcut target)               |
| `emoji-picker --show` / `--hide` | Explicit show or hide                     |
| `emoji-picker --version`         | Print version information                 |
| `emoji-picker --help`            | Print CLI help and options                |

Esc or focus loss hides the window; the process keeps running.

## Update emoji database

```bash
make data    # https://www.unicode.org/Public/emoji/latest/emoji-test.txt
make && make install
```

Pin a version: `UNICODE_VER=15.1 make data`.

## Docs (human-friendly)


| Doc                                          | About                                                                                   |
| -------------------------------------------- | --------------------------------------------------------------------------------------- |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | How the pieces fit                                                                      |
| [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md)   | Day-to-day hacking                                                                      |
| [docs/SHORTCUTS.md](docs/SHORTCUTS.md)       | Win+. on LXQt + Openbox                                                                 |
| [docs/SCREENSHOTS.md](docs/SCREENSHOTS.md)   | How screenshots were taken                                                              |
| [docs/RELEASES.md](docs/RELEASES.md)         | GitHub Releases publish/install                                                         |
| `[.agent/](.agent/)`                         | **All** AI-agent rules & context (Cursor, Antigravity, Kiro, Copilot, Claude, Graphify) |




## Project layout

```
emoji-picker/
├── .agent/
├── Makefile                # make / make test / make install
├── pack/
├── scripts/
├── tests/
├── src/
│   ├── main.c              # daemon, socket IPC
│   ├── ui/                 # GTK shell, grid, dismiss
│   ├── model/              # search, hits, history
│   ├── insert/             # clipboard paste into apps
│   ├── platform/           # X11 helpers
│   └── generated/          # DO NOT hand-edit
└── docs/
```



## Uninstall

```bash
make uninstall
# optionally remove shortcut bindings from Openbox / LXQt
```



## Limitations

- **X11 only** (GTK + xclip on X). Wayland needs a different path.
- Chromium needs clipboard paste (Unicode typing is ignored); insert uses
  `windowfocus` + Ctrl+V after releasing the GTK click grab.
- Skin-tone variants are omitted from the grid (base glyphs) to keep RAM/UI light.



## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) and [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md).
Security reports: [SECURITY.md](SECURITY.md).

Agent / automation notes: [AGENTS.md](AGENTS.md).

## License

This project is released under the [MIT License](LICENSE).

Emoji names and glyphs are derived from Unicode’s `emoji-test.txt` (fetched at
`make data`). Unicode data files are subject to the
[Unicode License](https://www.unicode.org/license.txt).

## Author

**Habibi Ahmad Aziz**

- Portfolio: [habibiahmada.dev](https://habibiahmada.dev)
- Email: [contact@habibiahmada.dev](mailto:contact@habibiahmada.dev)
- GitHub: [@habibiahmada](https://github.com/habibiahmada)



## Thanks

Thanks to everyone who contributes to this project.

![Contributors](https://contrib.rocks/image?repo=habibiahmada/emoji-picker)

Made with care for low-RAM Linux desktops.