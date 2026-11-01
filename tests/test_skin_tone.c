#include "model/skin_tone.h"
#include "test_harness.h"

#include <glib.h>
#include <string.h>
#include <unistd.h>

static char *g_tmpdir;

static int find_entry(const char *name) {
    for (int i = 0; i < EMOJI_COUNT; i++) {
        if (strcmp(emoji_entries[i].name, name) == 0)
            return i;
    }
    return -1;
}

static void test_variants_linked(void) {
    int thumbs = find_entry("thumbs up");
    TEST_ASSERT(thumbs >= 0);
    TEST_ASSERT(emoji_entries[thumbs].tones_count == 5);
    TEST_ASSERT(!skin_tone_has_pairs(thumbs));
    TEST_ASSERT(strcmp(skin_tone_uniform(thumbs, 5)->glyph, "👍🏿") == 0);

    int grin = find_entry("grinning face");
    TEST_ASSERT(emoji_entries[grin].tones_count == 0);
    TEST_ASSERT(skin_tone_uniform(grin, 3) == NULL);

    /* Two-person: uniform stored once, mixed pairs per person. */
    int shake = find_entry("handshake");
    TEST_ASSERT(emoji_entries[shake].tones_count == 25);
    TEST_ASSERT(skin_tone_has_pairs(shake));
    TEST_ASSERT(strcmp(skin_tone_pair(shake, 2, 2)->glyph, "🤝🏼") == 0);
    TEST_ASSERT(strcmp(skin_tone_pair(shake, 1, 2)->glyph,
                       "🫱🏻‍🫲🏼") == 0);

    /* 'kiss: person, person, <tones>' links to the plain 'kiss' base. */
    TEST_ASSERT(emoji_entries[find_entry("kiss")].tones_count == 25);
}

static void test_saved_tone_applies_everywhere(void) {
    skin_tone_load();
    TEST_ASSERT(skin_tone_get() == SKIN_TONE_UNSET);
    int thumbs = find_entry("thumbs up");
    int wave = find_entry("waving hand");
    TEST_ASSERT(strcmp(skin_tone_display_glyph(thumbs), "👍") == 0);

    skin_tone_set(4);
    TEST_ASSERT(strcmp(skin_tone_display_glyph(thumbs), "👍🏾") == 0);
    TEST_ASSERT(strcmp(skin_tone_display_glyph(wave), "👋🏾") == 0);

    /* Persists across reload. */
    skin_tone_load();
    TEST_ASSERT(skin_tone_get() == 4);

    skin_tone_set(SKIN_TONE_DEFAULT);
    skin_tone_load();
    TEST_ASSERT(skin_tone_get() == SKIN_TONE_DEFAULT);
    TEST_ASSERT(strcmp(skin_tone_display_glyph(thumbs), "👍") == 0);

    skin_tone_set(9);
    TEST_ASSERT(skin_tone_get() == SKIN_TONE_DEFAULT);
}

int main(void) {
    g_tmpdir = g_dir_make_tmp("emoji-picker-tone-XXXXXX", NULL);
    char *data = g_build_filename(g_tmpdir, "share", NULL);
    g_setenv("XDG_DATA_HOME", data, TRUE);

    TEST_RUN(test_variants_linked);
    TEST_RUN(test_saved_tone_applies_everywhere);

    char *file = g_build_filename(data, "emoji-picker", "skin-tone", NULL);
    char *dir = g_build_filename(data, "emoji-picker", NULL);
    unlink(file);
    rmdir(dir);
    rmdir(data);
    rmdir(g_tmpdir);
    g_free(file);
    g_free(dir);
    g_free(data);
    g_free(g_tmpdir);
    return test_finish();
}
