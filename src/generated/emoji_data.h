/* Auto-generated from Unicode emoji-test.txt — do not edit. */
#ifndef EMOJI_DATA_H
#define EMOJI_DATA_H

#include <stddef.h>
#include <stdint.h>

#define EMOJI_CATEGORY_COUNT 9
#define EMOJI_COUNT 1914
#define EMOJI_TONE_VARIANT_COUNT 2030

typedef struct {
    const char *glyph;    /* UTF-8 emoji */
    const char *name;     /* English name from Unicode (lowercase) */
    int category;         /* index into emoji_categories */
    uint16_t version;     /* Emoji version x10 (E13.1 → 131) */
    uint16_t tones_start; /* index into emoji_tone_variants */
    uint16_t tones_count; /* 0 = no skin-tone variants */
} EmojiEntry;

/* Skin-tone variant of a base entry, sorted by (tone1, tone2).
 * tone2 == 0: one tone for the whole emoji; else per-person tones. */
typedef struct {
    const char *glyph;
    uint8_t tone1; /* 1 = light … 5 = dark */
    uint8_t tone2;
    uint16_t version;
} EmojiToneVariant;

typedef struct {
    uint16_t start; /* index into emoji_entries */
    uint16_t count;
} EmojiCategoryRange;

extern const char *const emoji_categories[EMOJI_CATEGORY_COUNT];
extern const EmojiCategoryRange emoji_category_ranges[EMOJI_CATEGORY_COUNT];
extern const EmojiEntry emoji_entries[EMOJI_COUNT];
extern const EmojiToneVariant emoji_tone_variants[EMOJI_TONE_VARIANT_COUNT];

#endif
