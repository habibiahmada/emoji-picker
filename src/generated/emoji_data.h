/* Auto-generated from Unicode emoji-test.txt — do not edit. */
#ifndef EMOJI_DATA_H
#define EMOJI_DATA_H

#include <stddef.h>
#include <stdint.h>

#define EMOJI_CATEGORY_COUNT 9
#define EMOJI_COUNT 1914

typedef struct {
    const char *glyph; /* UTF-8 emoji */
    const char *name;  /* English name from Unicode (lowercase) */
    int category;      /* index into emoji_categories */
} EmojiEntry;

typedef struct {
    uint16_t start; /* index into emoji_entries */
    uint16_t count;
} EmojiCategoryRange;

extern const char *const emoji_categories[EMOJI_CATEGORY_COUNT];
extern const EmojiCategoryRange emoji_category_ranges[EMOJI_CATEGORY_COUNT];
extern const EmojiEntry emoji_entries[EMOJI_COUNT];

#endif
