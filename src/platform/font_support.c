#include "platform/font_support.h"

#include "generated/emoji_data.h"

#include <fontconfig/fontconfig.h>
#include <glib.h>

/* Same family the grid CSS asks for first. */
#define EMOJI_FONT_FAMILY "Noto Color Emoji"

static FcCharSet *emoji_charset(void) {
    FcPattern *pat = FcNameParse((const FcChar8 *)EMOJI_FONT_FAMILY);
    if (!pat)
        return NULL;
    FcConfigSubstitute(NULL, pat, FcMatchPattern);
    FcDefaultSubstitute(pat);
    FcResult res = FcResultNoMatch;
    FcPattern *match = FcFontMatch(NULL, pat, &res);
    FcPatternDestroy(pat);
    if (!match)
        return NULL;
    FcCharSet *cs = NULL, *copy = NULL;
    if (FcPatternGetCharSet(match, FC_CHARSET, 0, &cs) == FcResultMatch)
        copy = FcCharSetCopy(cs);
    FcPatternDestroy(match);
    return copy;
}

/* Lone codepoint of a glyph ignoring VS16, or 0 for sequences. */
static gunichar single_codepoint(const char *glyph) {
    gunichar cp = 0;
    for (const char *s = glyph; *s; s = g_utf8_next_char(s)) {
        gunichar c = g_utf8_get_char(s);
        if (c == 0xFE0F)
            continue;
        if (cp)
            return 0;
        cp = c;
    }
    return cp;
}

uint16_t font_emoji_max_version(void) {
    FcCharSet *cs = emoji_charset();
    if (!cs)
        return UINT16_MAX;
    /* Fallback matched a text font: no verdict. */
    if (!FcCharSetHasChar(cs, 0x1F600)) {
        FcCharSetDestroy(cs);
        return UINT16_MAX;
    }

    uint16_t first_missing = UINT16_MAX;
    for (int i = 0; i < EMOJI_COUNT; i++) {
        gunichar cp = single_codepoint(emoji_entries[i].glyph);
        if (cp && !FcCharSetHasChar(cs, cp) && emoji_entries[i].version < first_missing)
            first_missing = emoji_entries[i].version;
    }
    FcCharSetDestroy(cs);
    return first_missing == UINT16_MAX ? UINT16_MAX : (uint16_t)(first_missing - 1);
}
