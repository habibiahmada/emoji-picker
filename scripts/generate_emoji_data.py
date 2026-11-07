#!/usr/bin/env python3
"""Parse Unicode emoji-test.txt → compact C arrays for the picker.

Base emoji fill the grid. Skin-tone variants (modifiers 1F3FB–1F3FF) go into a
separate table linked from their base entry, so the grid stays one cell per
emoji and the tone popup / saved tone preference picks the variant.

Search keywords come from CLDR annotation files (any language) plus a small
hand-kept file for everyday words CLDR lacks (🗿 → rock, stone).
"""
from __future__ import annotations

import re
import sys
import unicodedata
import xml.etree.ElementTree as ET
from pathlib import Path

# Fitzpatrick skin-tone modifiers → tone 1..5
SKIN_TONES = {"1F3FB": 1, "1F3FC": 2, "1F3FD": 3, "1F3FE": 4, "1F3FF": 5}
SKIP_GROUPS = frozenset({"Component"})
TONE_PART_RE = re.compile(r"^(light|medium-light|medium|medium-dark|dark) skin tone$")
WORD_SPLIT_RE = re.compile(r"[^a-z0-9]+")
# Joining words that would only add noise matches.
STOPWORDS = frozenset({"and", "of", "the", "with", "dan", "di", "ke", "dengan", "yang", "untuk"})


def glyph_key(glyph: str) -> str:
    """CLDR keys drop VS16; match on the same form."""
    return glyph.replace("\ufe0f", "")


def ascii_words(text: str) -> list[str]:
    """'Café’s fire-truck' → ['cafes', 'fire', 'truck'] (search is ASCII)."""
    text = unicodedata.normalize("NFKD", text.lower())
    text = "".join(c for c in text if not unicodedata.combining(c))
    text = text.replace("\u2019", "").replace("'", "")
    return [w for w in WORD_SPLIT_RE.split(text) if len(w) >= 2 and w not in STOPWORDS]


def load_cldr(path: Path, out: dict[str, list[str]]) -> None:
    """Keywords and the localized name (type="tts") of every annotation."""
    for ann in ET.parse(path).getroot().iter("annotation"):
        if ann.text:
            out.setdefault(glyph_key(ann.get("cp", "")), []).extend(ascii_words(ann.text))


def load_extra(path: Path, out: dict[str, list[str]]) -> list[str]:
    """Lines of '<emoji> ; word word …'; '#' starts a comment. Returns keys."""
    keys = []
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.split("#", 1)[0].strip()
        if not line:
            continue
        glyph, _, words = line.partition(";")
        keys.append(glyph_key(glyph.strip()))
        out.setdefault(keys[-1], []).extend(ascii_words(words))
    return keys


def keywords_for(name: str, words: list[str]) -> str:
    """Unique keywords not already in the name, first source first."""
    seen = set(ascii_words(name))
    kept = []
    for w in words:
        if w not in seen:
            seen.add(w)
            kept.append(w)
    return " ".join(kept)


def base_name(name: str) -> str:
    """'kiss: woman, man, light skin tone' → 'kiss: woman, man'."""
    if ": " not in name:
        return name
    head, tail = name.split(": ", 1)
    parts = [x for x in tail.split(", ") if not TONE_PART_RE.match(x)]
    return head + (": " + ", ".join(parts) if parts else "")


def emoji_version(v: str) -> int:
    """'13.1' → 131, '0.6' → 6."""
    return int(round(float(v) * 10))


def c_escape(s: str) -> str:
    return (
        s.replace("\\", "\\\\")
        .replace('"', '\\"')
        .replace("\n", "\\n")
    )


def main() -> int:
    args = sys.argv[1:]
    extra: Path | None = None
    if "--keywords" in args:
        i = args.index("--keywords")
        extra = Path(args[i + 1])
        del args[i : i + 2]
    if len(args) < 3:
        print(
            f"usage: {sys.argv[0]} emoji-test.txt out.h out.c "
            "[--keywords extra.txt] [cldr-annotations.xml ...]",
            file=sys.stderr,
        )
        return 2

    src = Path(args[0])
    out_h = Path(args[1])
    out_c = Path(args[2])

    keyword_src: dict[str, list[str]] = {}
    for path in args[3:]:
        load_cldr(Path(path), keyword_src)
    extra_keys = load_extra(extra, keyword_src) if extra else []

    group = "Other"
    categories: list[str] = []
    cat_index: dict[str, int] = {}
    emojis: list[tuple[str, str, int, int]] = []  # utf8, name, cat_id, version
    # base name → [(tone1, tone2, utf8, version)]
    variants: dict[str, list[tuple[int, int, str, int]]] = {}

    group_re = re.compile(r"^# group:\s*(.+)$")
    # 1F600 ; fully-qualified # 😀 E1.0 grinning face
    line_re = re.compile(
        r"^([0-9A-F ]+)\s*;\s*fully-qualified\s*#\s*(\S+)\s+E([0-9.]+)\s+(.+)$"
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
        glyph, ver = m.group(2), emoji_version(m.group(3))
        name = m.group(4).strip().lower()
        tones = [SKIN_TONES[cp] for cp in cps if cp in SKIN_TONES]
        if tones:
            t1 = tones[0]
            t2 = tones[1] if len(tones) > 1 else 0
            variants.setdefault(base_name(name), []).append((t1, t2, glyph, ver))
            continue
        if group not in cat_index:
            cat_index[group] = len(categories)
            categories.append(group)
        emojis.append((glyph, name, cat_index[group], ver))

    # Link variants to base entries. 'kiss: person, person, <tones>' has no
    # base of that name — fall back to the head ('kiss').
    by_name = {e[1]: i for i, e in enumerate(emojis)}
    tone_rows: list[tuple[int, int, str, int]] = []
    tone_link: dict[int, tuple[int, int]] = {}  # base idx → (start, count)
    linked: dict[int, list[tuple[int, int, str, int]]] = {}
    for bname, vs in variants.items():
        idx = by_name.get(bname)
        if idx is None:
            idx = by_name.get(bname.split(": ", 1)[0])
        if idx is None:
            print(f"warning: no base for tone variants of {bname!r}", file=sys.stderr)
            continue
        linked.setdefault(idx, []).extend(vs)
    for idx in sorted(linked):
        vs = sorted(linked[idx])
        tone_link[idx] = (len(tone_rows), len(vs))
        tone_rows.extend(vs)

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
#define EMOJI_TONE_VARIANT_COUNT {len(tone_rows)}

typedef struct {{
    const char *glyph;    /* UTF-8 emoji */
    const char *name;     /* English name from Unicode (lowercase) */
    const char *keywords; /* extra ASCII search words, space-separated */
    int category;         /* index into emoji_categories */
    uint16_t version;     /* Emoji version x10 (E13.1 → 131) */
    uint16_t tones_start; /* index into emoji_tone_variants */
    uint16_t tones_count; /* 0 = no skin-tone variants */
}} EmojiEntry;

/* Skin-tone variant of a base entry, sorted by (tone1, tone2).
 * tone2 == 0: one tone for the whole emoji; else per-person tones. */
typedef struct {{
    const char *glyph;
    uint8_t tone1; /* 1 = light … 5 = dark */
    uint8_t tone2;
    uint16_t version;
}} EmojiToneVariant;

typedef struct {{
    uint16_t start; /* index into emoji_entries */
    uint16_t count;
}} EmojiCategoryRange;

extern const char *const emoji_categories[EMOJI_CATEGORY_COUNT];
extern const EmojiCategoryRange emoji_category_ranges[EMOJI_CATEGORY_COUNT];
extern const EmojiEntry emoji_entries[EMOJI_COUNT];
extern const EmojiToneVariant emoji_tone_variants[EMOJI_TONE_VARIANT_COUNT];

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
    with_keywords = 0
    known = {glyph_key(e[0]) for e in emojis}
    for key in extra_keys:
        if key not in known:
            print(f"warning: extra keywords for unknown emoji {key!r}", file=sys.stderr)
    for i, (glyph, name, cat, ver) in enumerate(emojis):
        start, count = tone_link.get(i, (0, 0))
        kw = keywords_for(name, keyword_src.get(glyph_key(glyph), []))
        with_keywords += bool(kw)
        lines.append(
            f'    {{"{c_escape(glyph)}", "{c_escape(name)}", "{c_escape(kw)}", '
            f"{cat}, {ver}, {start}, {count}}},"
        )
    lines.append("};")
    lines.append("")
    lines.append(
        "const EmojiToneVariant emoji_tone_variants[EMOJI_TONE_VARIANT_COUNT] = {"
    )
    for t1, t2, glyph, ver in tone_rows:
        lines.append(f'    {{"{c_escape(glyph)}", {t1}, {t2}, {ver}}},')
    lines.append("};")
    lines.append("")
    out_c.write_text("\n".join(lines) + "\n", encoding="utf-8")

    print(
        f"categories={len(categories)} emojis={len(emojis)} "
        f"tone_variants={len(tone_rows)} with_tones={len(tone_link)} "
        f"with_keywords={with_keywords}",
        file=sys.stderr,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
