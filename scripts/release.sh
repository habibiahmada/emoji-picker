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
install -m 644 pack/emoji-picker.service "$STAGE/emoji-picker.service"
install -m 644 README.md "$STAGE/README.md"
install -m 644 LICENSE "$STAGE/LICENSE"

cat > "$STAGE/INSTALL.txt" <<EOF
emoji-picker ${VERSION} — Linux ${ARCH_LABEL}

Quick install (user-local):

  mkdir -p ~/.local/bin ~/.config/systemd/user
  install -m 755 emoji-picker ~/.local/bin/emoji-picker
  install -m 644 emoji-picker.service ~/.config/systemd/user/emoji-picker.service

  # If the popup never appears under LXQt, add a display drop-in:
  # ~/.config/systemd/user/emoji-picker.service.d/display.conf
  #   [Service]
  #   Environment=DISPLAY=:0
  #   Environment=XAUTHORITY=%h/.Xauthority

  systemctl --user daemon-reload
  systemctl --user enable --now emoji-picker.service
  emoji-picker --toggle

Runtime deps (Debian/Ubuntu/Lubuntu):
  sudo apt install libgtk-3-0 xdotool fonts-noto-color-emoji

Bind Win+. (Meta+period) — see README.md / docs/SHORTCUTS.md
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
## emoji-picker ${TAG}

Lightweight Unicode emoji popup for Linux **X11** (C + GTK3).

### Assets

| File | What |
|------|------|
| \`${STAGE_NAME}.tar.gz\` | Prebuilt binary + user systemd unit + INSTALL.txt |
| \`*.sha256\` | Checksum |

### Install

See \`INSTALL.txt\` inside the tarball (or README). Needs GTK3, \`xdotool\`, and a color-emoji font.

### Notes

- X11 only (insert via \`xdotool type\`)
- Multi-pick stays open; Esc / click-outside to hide
- Default toggle shortcut: **Win+.** (configure in LXQt / Openbox)
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
