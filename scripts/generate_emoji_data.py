#!/usr/bin/env python3
"""Parse Unicode emoji-test.txt → compact C arrays for the picker.

Skips skin-tone variants (modifiers 1F3FB–1F3FF) and empty/Component groups
so the UI stays small; search still covers English names of base emoji.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

# Fitzpatrick skin-tone modifiers
SKIN_TONES = frozenset({"1F3FB", "1F3FC", "1F3FD", "1F3FE", "1F3FF"})
SKIP_GROUPS = frozenset({"Component"})


def c_escape(s: str) -> str:
    return (
        s.replace("\\", "\\\\")
        .replace('"', '\\"')
        .replace("\n", "\\n")
    )


def main() -> int:
    if len(sys.argv) != 4:
        print(
            f"usage: {sys.argv[0]} emoji-test.txt out.h out.c",
            file=sys.stderr,
        )
        return 2

    src = Path(sys.argv[1])
    out_h = Path(sys.argv[2])
    out_c = Path(sys.argv[3])

    group = "Other"
    categories: list[str] = []
    cat_index: dict[str, int] = {}
    emojis: list[tuple[str, str, int]] = []  # utf8, name, cat_id
    skipped_skin = 0

    group_re = re.compile(r"^# group:\s*(.+)$")
    # 1F600 ; fully-qualified # 😀 E1.0 grinning face
    line_re = re.compile(
        r"^([0-9A-F ]+)\s*;\s*fully-qualified\s*#\s*(\S+)\s+E[0-9.]+\s+(.+)$"
    )

    for raw in src.read_text(encoding="utf-8").splitlines():
        m = group_re.match(raw)
        if m:
            group = m.group(1).strip()
            continue
        m = line_re.match(raw)
        if not m:
            continue
        if group in SKIP_GROUPS:
            continue
        cps = m.group(1).strip().split()
        if any(cp in SKIN_TONES for cp in cps):
            skipped_skin += 1
            continue
        glyph, name = m.group(2), m.group(3).strip().lower()
        if group not in cat_index:
            cat_index[group] = len(categories)
            categories.append(group)
        emojis.append((glyph, name, cat_index[group]))

    # Contiguous ranges per category (entries are emitted in group order)
    ranges: list[tuple[int, int]] = [(0, 0)] * len(categories)
    i = 0
    while i < len(emojis):
        cat = emojis[i][2]
        start = i
        while i < len(emojis) and emojis[i][2] == cat:
            i += 1
        ranges[cat] = (start, i - start)

    out_h.parent.mkdir(parents=True, exist_ok=True)
    out_h.write_text(
        f"""/* Auto-generated from Unicode emoji-test.txt — do not edit. */
#ifndef EMOJI_DATA_H
#define EMOJI_DATA_H

#include <stddef.h>
#include <stdint.h>

#define EMOJI_CATEGORY_COUNT {len(categories)}
#define EMOJI_COUNT {len(emojis)}

typedef struct {{
    const char *glyph; /* UTF-8 emoji */
    const char *name;  /* English name from Unicode (lowercase) */
    int category;      /* index into emoji_categories */
}} EmojiEntry;

typedef struct {{
    uint16_t start; /* index into emoji_entries */
    uint16_t count;
}} EmojiCategoryRange;

extern const char *const emoji_categories[EMOJI_CATEGORY_COUNT];
extern const EmojiCategoryRange emoji_category_ranges[EMOJI_CATEGORY_COUNT];
extern const EmojiEntry emoji_entries[EMOJI_COUNT];

#endif
""",
        encoding="utf-8",
    )

    lines = [
        "/* Auto-generated from Unicode emoji-test.txt — do not edit. */",
        '#include "emoji_data.h"',
        "",
        "const char *const emoji_categories[EMOJI_CATEGORY_COUNT] = {",
    ]
    for c in categories:
        lines.append(f'    "{c_escape(c)}",')
    lines.append("};")
    lines.append("")
    lines.append(
        "const EmojiCategoryRange emoji_category_ranges[EMOJI_CATEGORY_COUNT] = {"
    )
    for start, count in ranges:
        lines.append(f"    {{{start}, {count}}},")
    lines.append("};")
    lines.append("")
    lines.append("const EmojiEntry emoji_entries[EMOJI_COUNT] = {")
    for glyph, name, cat in emojis:
        lines.append(
            f'    {{"{c_escape(glyph)}", "{c_escape(name)}", {cat}}},'
        )
    lines.append("};")
    lines.append("")
    out_c.write_text("\n".join(lines) + "\n", encoding="utf-8")

    print(
        f"categories={len(categories)} emojis={len(emojis)} "
        f"skipped_skin={skipped_skin}",
        file=sys.stderr,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
