#!/usr/bin/env bash
# Fetch latest Unicode emoji-test.txt plus CLDR search keywords and generate
# compact data for the picker.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DATA_DIR="$ROOT/data"
GEN_DIR="$ROOT/src/generated"
UNICODE_VER="${UNICODE_VER:-latest}"
# CLDR release whose annotations match the Unicode emoji version.
CLDR_VER="${CLDR_VER:-release-49}"
CLDR_LANGS="en id"

mkdir -p "$DATA_DIR/cldr" "$GEN_DIR"

if [[ "$UNICODE_VER" == "latest" ]]; then
  # Resolve current Unicode emoji draft/release folder via emoji-test in latest
  URL="https://www.unicode.org/Public/emoji/latest/emoji-test.txt"
else
  URL="https://www.unicode.org/Public/emoji/${UNICODE_VER}/emoji-test.txt"
fi

echo "Fetching $URL ..."
curl -fsSL "$URL" -o "$DATA_DIR/emoji-test.txt"

CLDR_URL="https://raw.githubusercontent.com/unicode-org/cldr/${CLDR_VER}/common"
annotations=()
for lang in $CLDR_LANGS; do
  for kind in annotations annotationsDerived; do
    out="$DATA_DIR/cldr/${kind}-${lang}.xml"
    echo "Fetching ${CLDR_VER} ${kind}/${lang}.xml ..."
    curl -fsSL "$CLDR_URL/${kind}/${lang}.xml" -o "$out"
    annotations+=("$out")
  done
done

python3 "$ROOT/scripts/generate_emoji_data.py" \
  "$DATA_DIR/emoji-test.txt" \
  "$GEN_DIR/emoji_data.h" \
  "$GEN_DIR/emoji_data.c" \
  --keywords "$DATA_DIR/keywords-extra.txt" \
  "${annotations[@]}"

echo "Generated:"
echo "  $GEN_DIR/emoji_data.h"
echo "  $GEN_DIR/emoji_data.c"
wc -l "$DATA_DIR/emoji-test.txt" "$GEN_DIR/emoji_data.c"
