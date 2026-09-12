#!/usr/bin/env bash
# Fetch latest Unicode emoji-test.txt and generate compact data for the picker.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DATA_DIR="$ROOT/data"
GEN_DIR="$ROOT/src/generated"
UNICODE_VER="${UNICODE_VER:-latest}"

mkdir -p "$DATA_DIR" "$GEN_DIR"

if [[ "$UNICODE_VER" == "latest" ]]; then
  # Resolve current Unicode emoji draft/release folder via emoji-test in latest
  URL="https://www.unicode.org/Public/emoji/latest/emoji-test.txt"
else
  URL="https://www.unicode.org/Public/emoji/${UNICODE_VER}/emoji-test.txt"
fi

echo "Fetching $URL ..."
curl -fsSL "$URL" -o "$DATA_DIR/emoji-test.txt"

python3 "$ROOT/scripts/generate_emoji_data.py" \
  "$DATA_DIR/emoji-test.txt" \
  "$GEN_DIR/emoji_data.h" \
  "$GEN_DIR/emoji_data.c"

echo "Generated:"
echo "  $GEN_DIR/emoji_data.h"
echo "  $GEN_DIR/emoji_data.c"
wc -l "$DATA_DIR/emoji-test.txt" "$GEN_DIR/emoji_data.c"
