#!/usr/bin/env bash
# Capture README screenshots over the real LXQt wallpaper (not the IDE).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
OUT="$ROOT/docs/screenshots"
WALLPAPER="${WALLPAPER:-$HOME/.local/share/lxqt/themes/LQXT-Onyxian-Rounded/wallpapers/Onyxian-Rounded.png}"
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

mkdir -p "$OUT"

minimize_busy() {
  local id name
  while read -r id; do
    name=$(xdotool getwindowname "$id" 2>/dev/null || true)
    case "$name" in
      *Cursor*|cursor|*brave*|*Brave*|*qterminal*|*Chromium*|*Untitled*)
        xdotool windowminimize "$id" 2>/dev/null || true
        ;;
    esac
  done < <(xdotool search --name '.*' 2>/dev/null || true)
}

picker_id() {
  xdotool search --name '^Emoji$' 2>/dev/null | tail -n1
}

show_picker() {
  emoji-picker --hide 2>/dev/null || true
  sleep 0.2
  emoji-picker --show || true
  sleep 0.7
  local id
  id=$(picker_id)
  if [[ -z "$id" ]]; then
    echo "emoji window not found" >&2
    exit 1
  fi
  xdotool windowactivate --sync "$id"
  xdotool windowmove --sync "$id" 750 260
  xdotool windowraise "$id"
  sleep 0.2
  echo "$id"
}

# Click relative to picker window (inner client coords work with --window)
click_picker() {
  local id=$1 x=$2 y=$3
  xdotool windowactivate --sync "$id"
  xdotool mousemove --sync --window "$id" "$x" "$y"
  sleep 0.05
  xdotool click 1
  sleep 0.25
}

type_search() {
  local id=$1 text=$2
  # Search entry ~ center of top field
  click_picker "$id" 200 28
  sleep 0.15
  xdotool key --clearmodifiers ctrl+a
  sleep 0.05
  xdotool key --clearmodifiers BackSpace
  sleep 0.1
  xdotool type --clearmodifiers --delay 40 -- "$text"
  # debounce search 60ms + fill
  sleep 0.9
}

grab_window() {
  local id=$1 dest=$2
  # Re-raise before grab so decorations/state are settled
  xdotool windowactivate --sync "$id"
  sleep 0.15
  import -window "$id" "$dest"
}

minimize_busy
sleep 0.35

convert "$WALLPAPER" -resize 1920x1080^ -gravity center -extent 1920x1080 \
  "$TMP/desktop-base.png"

ID=$(show_picker)

# Tabs (approx): History~28, All~55, Smileys~95, People~130, Animals~165
# Overview: All tab with full grid
click_picker "$ID" 55 58
sleep 0.6
grab_window "$ID" "$TMP/picker-overview-raw.png"
convert "$TMP/picker-overview-raw.png" -bordercolor '#1b1e24' -border 1 \
  "$OUT/picker-hero.png"
cp "$TMP/picker-overview-raw.png" "$OUT/picker-overview.png"

# Search smile / fire / cat for docs
type_search "$ID" "fire"
grab_window "$ID" "$OUT/picker-search.png"

type_search "$ID" "cat"
grab_window "$ID" "$OUT/picker-search-cat.png"

# Desktop context: clear search, All tab, composite on wallpaper
click_picker "$ID" 200 28
xdotool key --clearmodifiers ctrl+a BackSpace
sleep 0.2
click_picker "$ID" 55 58
sleep 0.6
grab_window "$ID" "$TMP/picker-desktop-raw.png"

convert "$TMP/desktop-base.png" \
  \( "$TMP/picker-desktop-raw.png" \
     \( +clone -background black -shadow 55x10+6+8 \) \
     +swap -background none -layers merge +repage \) \
  -geometry +750+260 -compose over -composite \
  "$OUT/desktop-with-picker.png"

convert "$OUT/desktop-with-picker.png" -crop 780x560+680+180 +repage \
  "$OUT/picker-in-context.png"

emoji-picker --hide 2>/dev/null || true

echo "Updated:"
identify "$OUT"/*.png
