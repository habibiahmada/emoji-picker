#ifndef SKIN_TONE_H
#define SKIN_TONE_H

#include <stdint.h>

#include "generated/emoji_data.h"

#define SKIN_TONE_UNSET (-1) /* never chosen: first click opens the tone popup */
#define SKIN_TONE_DEFAULT 0  /* chosen: plain yellow */
#define SKIN_TONE_MAX 5      /* 1 = light … 5 = dark */

/* One saved tone applies to every emoji that has skin-tone variants.
 * Stored in ~/.local/share/emoji-picker/skin-tone. */
void skin_tone_load(void);
int skin_tone_get(void);
void skin_tone_set(int tone);

/* Variant of base entry `idx` with `tone` for every person, or NULL. */
const EmojiToneVariant *skin_tone_uniform(int idx, int tone);
/* Variant with per-person tones (t1, t2), or NULL. t1 == t2 → uniform. */
const EmojiToneVariant *skin_tone_pair(int idx, int t1, int t2);
/* TRUE when the entry has per-person (two-tone) variants. */
int skin_tone_has_pairs(int idx);

/* Glyph to show/insert for base entry `idx` under the saved tone. */
const char *skin_tone_display_glyph(int idx);

#endif
