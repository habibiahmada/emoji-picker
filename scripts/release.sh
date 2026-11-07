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

## ⚡ Quick Install (One Line)

No clone, no compiler: downloads this release, checks its SHA-256 and installs it to \`~/.local\`:

\`\`\`bash
curl -fsSL https://raw.githubusercontent.com/habibiahmada/emoji-picker/main/get.sh | sh
\`\`\`

Or by hand: download \`${STAGE_NAME}.tar.gz\` below, then \`tar -xzf ${STAGE_NAME}.tar.gz && cd ${STAGE_NAME} && ./install.sh\`.

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

### 🔎 Search that understands everyday words
- Search matches keywords, not only the official Unicode name: \`rock\`, \`stone\` or \`batu\` find 🗿, \`fire\` lists 🔥 first.
- Indonesian works too (\`ngakak\`, \`jempol\`, \`geleng\`, \`kucing\`), from Unicode CLDR keywords plus a small list for slang (\`wkwk\`, \`galau\`, \`santuy\`, \`otw\`).
- Results are ranked: exact name, then name word, then keyword, then prefixes.

### 📦 One-line install
- \`curl -fsSL https://raw.githubusercontent.com/habibiahmada/emoji-picker/main/get.sh | sh\` installs the latest release: no clone, no compiler, SHA-256 verified. Run it again to update.

### 🆕 Unicode 18
- 9 new emoji (e.g. cracking face). They appear once your color emoji font supports them.

### 🐛 Fixes
- \`emoji-picker --show\` no longer hides an already open picker.

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
