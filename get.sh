#!/bin/sh
# ==============================================================================
# emoji-picker one-line installer (no clone, no build tools):
#
#   curl -fsSL https://raw.githubusercontent.com/habibiahmada/emoji-picker/main/get.sh | sh
#
# Downloads the latest release tarball, checks its SHA-256, runs the bundled
# install.sh (binary in ~/.local/bin, systemd user service, Win+. shortcut) and
# removes the download. Re-run to update. Options go to install.sh:
#
#   curl -fsSL .../get.sh | sh -s -- --uninstall
#
# Environment:
#   EMOJI_PICKER_VERSION   install this version (e.g. 0.4.0) instead of latest
#   EMOJI_PICKER_BASE_URL  release download base (tests point it at file://)
# ==============================================================================

REPO="habibiahmada/emoji-picker"

say() { printf '%s\n' "emoji-picker: $*"; }
die() { printf '%s\n' "emoji-picker: error: $*" >&2; exit 1; }

# fetch URL OUTFILE ("-" = stdout)
fetch() {
  if command -v curl >/dev/null 2>&1; then
    curl -fsSL --connect-timeout 10 -o "$2" "$1"
  elif command -v wget >/dev/null 2>&1; then
    wget -q -T 10 -O "$2" "$1"
  else
    die "curl or wget is required"
  fi
}

latest_version() {
  # "tag_name": "v0.4.0" → 0.4.0
  fetch "https://api.github.com/repos/$REPO/releases/latest" - 2>/dev/null |
    sed -n 's/.*"tag_name"[[:space:]]*:[[:space:]]*"v\{0,1\}\([^"]*\)".*/\1/p' |
    head -n 1
}

# Chosen up front: a die() inside the verify subshell would not stop main.
pick_sha256() {
  if command -v sha256sum >/dev/null 2>&1; then
    SHA256="sha256sum"
  elif command -v shasum >/dev/null 2>&1; then
    SHA256="shasum -a 256"
  else
    die "sha256sum or shasum is required to verify the download"
  fi
}

main() {
  [ "$(uname -s)" = Linux ] || die "only Linux (X11) is supported"
  case "$(uname -m)" in
    x86_64 | amd64) arch=x86_64 ;;
    *) die "no prebuilt binary for $(uname -m); build from source: https://github.com/$REPO#from-source" ;;
  esac
  command -v bash >/dev/null 2>&1 || die "bash is required"
  command -v tar >/dev/null 2>&1 || die "tar is required"
  pick_sha256

  version="${EMOJI_PICKER_VERSION:-}"
  if [ -z "$version" ]; then
    version="$(latest_version)"
    [ -n "$version" ] || die "could not find the latest release (offline or GitHub API limit); set EMOJI_PICKER_VERSION"
  fi
  version="${version#v}"

  base="${EMOJI_PICKER_BASE_URL:-https://github.com/$REPO/releases/download/v$version}"
  name="emoji-picker-$version-linux-$arch"

  tmp="$(mktemp -d)" || die "cannot create a temporary directory"
  trap 'rm -rf "$tmp"' EXIT
  trap 'exit 130' INT TERM

  say "downloading v$version ($arch)"
  fetch "$base/$name.tar.gz" "$tmp/$name.tar.gz" || die "download failed: $base/$name.tar.gz"
  fetch "$base/$name.sha256" "$tmp/$name.sha256" || die "download failed: $base/$name.sha256"
  (cd "$tmp" && $SHA256 -c "$name.sha256" >/dev/null 2>&1) ||
    die "checksum mismatch for $name.tar.gz"

  tar -xzf "$tmp/$name.tar.gz" -C "$tmp" || die "cannot extract $name.tar.gz"
  [ -f "$tmp/$name/install.sh" ] || die "installer missing from $name.tar.gz"

  # stdin is this script when piped; the installer must not read it.
  bash "$tmp/$name/install.sh" "$@" </dev/null
}

main "$@"
