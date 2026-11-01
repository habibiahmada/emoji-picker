#!/usr/bin/env bash
# Build release assets and (optionally) publish a GitHub Release.
#
# Usage:
#   ./scripts/release.sh 0.1.0           # build assets only → dist/
#   ./scripts/release.sh 0.1.0 --publish # build + gh release create/upload
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

VERSION="${1:-}"
PUBLISH=0
if [[ "${2:-}" == "--publish" ]] || [[ "${1:-}" == "--publish" ]]; then
  PUBLISH=1
  if [[ "${1:-}" == "--publish" ]]; then
    VERSION="${2:-}"
  fi
fi

if [[ -z "$VERSION" ]]; then
  echo "usage: $0 <version> [--publish]" >&2
  echo "  example: $0 0.1.0 --publish" >&2
  exit 2
fi

VERSION="${VERSION#v}"
TAG="v${VERSION}"
ARCH="$(uname -m)"
case "$ARCH" in
  x86_64|amd64) ARCH_LABEL=x86_64 ;;
  aarch64|arm64) ARCH_LABEL=aarch64 ;;
  *) ARCH_LABEL="$ARCH" ;;
esac

DIST="$ROOT/dist"
STAGE_NAME="emoji-picker-${VERSION}-linux-${ARCH_LABEL}"
STAGE="$DIST/${STAGE_NAME}"
TARBALL="$DIST/${STAGE_NAME}.tar.gz"

echo "==> Building emoji-picker ${VERSION} (${ARCH_LABEL})"
make clean
make CFLAGS="-O2 -pipe -Wall -Wextra -DNDEBUG"

rm -rf "$STAGE"
mkdir -p "$STAGE"

install -m 755 emoji-picker "$STAGE/emoji-picker"
install -m 755 install.sh "$STAGE/install.sh"
install -m 644 pack/emoji-picker.service "$STAGE/emoji-picker.service"
install -m 644 README.md "$STAGE/README.md"
install -m 644 LICENSE "$STAGE/LICENSE"

cat > "$STAGE/INSTALL.txt" <<EOF
emoji-picker ${VERSION} — Linux ${ARCH_LABEL}

Quick install (one step):

  ./install.sh

This automatically installs the binary, configures the systemd user
service (with display env), and registers the Win+. (Super+Period) shortcut.

Options:
  ./install.sh --help
  ./install.sh --dry-run
  ./install.sh --uninstall

Runtime deps (Debian/Ubuntu/Lubuntu):
  sudo apt install libgtk-3-0 xdotool fonts-noto-color-emoji xclip
EOF

tar -C "$DIST" -czf "$TARBALL" "$STAGE_NAME"
(
  cd "$DIST"
  sha256sum "${STAGE_NAME}.tar.gz" > "${STAGE_NAME}.sha256"
)

echo "==> Assets:"
ls -lah "$TARBALL" "$DIST/${STAGE_NAME}.sha256"

if [[ "$PUBLISH" -ne 1 ]]; then
  echo "Build only. Re-run with --publish to create GitHub Release ${TAG}."
  exit 0
fi

if ! command -v gh >/dev/null; then
  echo "gh CLI required for --publish" >&2
  exit 1
fi

NOTES="$DIST/release-notes-${VERSION}.md"
cat > "$NOTES" <<EOF
# emoji-picker ${TAG} 🎉

A tiny **C + GTK3** Unicode emoji popup for Linux **X11** desktops (LXQt, Openbox, GNOME, XFCE, i3, Sway).
Fast, memory-efficient (~30 MB RSS resident), warm daemon for instant **Win+.** toggle, multi-pick support, and direct paste into the pre-picker window.

---

## ⚡ Quick Install (One-Step)

Download and extract the release asset, then run the installer:

\`\`\`bash
tar -xzf ${STAGE_NAME}.tar.gz
cd ${STAGE_NAME}
./install.sh
\`\`\`

**Done!** Press **Win + .** (\`Super+Period\`) anywhere to open the emoji picker.

### What \`./install.sh\` does automatically:
1. **Verifies runtime dependencies** (\`xdotool\`, \`xclip\`, color emoji fonts) and suggests package manager commands if missing.
2. **Installs binary** to \`~/.local/bin/emoji-picker\`.
3. **Installs & enables systemd user service** with proper \`DISPLAY\` and \`XAUTHORITY\` drop-in configuration.
4. **Configures the global shortcut (Win+.)** automatically for your desktop environment:
   - **LXQt**: registers in \`globalkeyshortcuts.conf\` and reloads \`lxqt-globalkeysd\`.
   - **Openbox**: registers in \`rc.xml\` and reloads with \`openbox --reconfigure\`.
   - **GNOME**: registers custom keybinding via \`gsettings\`.
   - **XFCE**: sets keybinding via \`xfconf-query\`.
   - **i3 / Sway**: appends bindsym to config and reloads.
5. **Verifies installation** and starts the daemon immediately.

---

## 🚀 What's New in ${TAG}

- **Unified One-Step Installer (\`install.sh\`)**: Replace multi-step setup with a single automated installer that handles dependencies, compilation, systemd service, and shortcuts.
- **Auto-Configured Desktop Shortcuts**: Zero manual editing needed for Win+. on Openbox, LXQt, GNOME, XFCE, and i3/Sway.
- **Built-in Self-Update Mechanism**: Check and update anytime using \`./install.sh --check-update\` or \`./install.sh --update\` (or \`make update\`).
- **CLI Options**: Added \`--version\` and \`--help\` flags to the binary.
- **Automated Test Suite**: Added 22 automated installer and update test cases integrated into \`make test\`.

---

## ⌨️ Usage

| Command | Action |
|---|---|
| \`emoji-picker --toggle\` | Toggle picker popup (default shortcut target) |
| \`emoji-picker --show\` | Explicitly show popup |
| \`emoji-picker --hide\` | Explicitly hide popup |
| \`emoji-picker --version\` | Display version information |
| \`emoji-picker --help\` | Display command-line options |

---

## 📦 Assets & Checksums

| File | Description |
|---|---|
| \`${STAGE_NAME}.tar.gz\` | Prebuilt binary + installer + systemd unit + README |
| \`${STAGE_NAME}.sha256\` | SHA-256 Checksum |
EOF

# Create tag locally if missing, push tag, then release
if ! git rev-parse "$TAG" >/dev/null 2>&1; then
  git tag -a "$TAG" -m "Release ${TAG}"
fi
git push origin "$TAG"

if gh release view "$TAG" >/dev/null 2>&1; then
  echo "Release ${TAG} exists — uploading assets…"
  gh release upload "$TAG" \
    "$TARBALL" \
    "$DIST/${STAGE_NAME}.sha256" \
    --clobber
else
  gh release create "$TAG" \
    "$TARBALL" \
    "$DIST/${STAGE_NAME}.sha256" \
    --title "emoji-picker ${TAG}" \
    --notes-file "$NOTES" \
    --latest
fi

echo "==> Published: $(gh release view "$TAG" --json url -q .url)"
