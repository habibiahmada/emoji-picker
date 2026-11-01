#include "model/skin_tone.h"

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>

static int g_tone = SKIN_TONE_UNSET;

static char *skin_tone_path(void) {
    return g_build_filename(g_get_user_data_dir(), "emoji-picker", "skin-tone",
                            NULL);
}

void skin_tone_load(void) {
    g_tone = SKIN_TONE_UNSET;
    char *path = skin_tone_path();
    char *text = NULL;
    if (g_file_get_contents(path, &text, NULL, NULL)) {
        char *end = NULL;
        long v = strtol(text, &end, 10);
        if (end != text && v >= SKIN_TONE_DEFAULT && v <= SKIN_TONE_MAX)
            g_tone = (int)v;
        g_free(text);
    }
    g_free(path);
}

int skin_tone_get(void) {
    return g_tone;
}

void skin_tone_set(int tone) {
    if (tone < SKIN_TONE_DEFAULT || tone > SKIN_TONE_MAX || tone == g_tone)
        return;
    g_tone = tone;

    char *dir = g_build_filename(g_get_user_data_dir(), "emoji-picker", NULL);
    g_mkdir_with_parents(dir, 0700);
    g_free(dir);
    char *path = skin_tone_path();
    char buf[8];
    g_snprintf(buf, sizeof(buf), "%d\n", tone);
    /* No fsync: it stalled the UI ~40ms under swap; losing a 2-byte
     * preference on power loss only brings the tone popup back. */
    g_file_set_contents_full(path, buf, -1, G_FILE_SET_CONTENTS_NONE, 0600, NULL);
    g_free(path);
}

const EmojiToneVariant *skin_tone_pair(int idx, int t1, int t2) {
    if (idx < 0 || idx >= EMOJI_COUNT || t1 < 1 || t1 > SKIN_TONE_MAX)
        return NULL;
    const EmojiEntry *e = &emoji_entries[idx];
    for (int i = 0; i < e->tones_count; i++) {
        const EmojiToneVariant *v = &emoji_tone_variants[e->tones_start + i];
        /* Same tone for both people is stored once, as tone2 == 0. */
        int want2 = t2 == t1 ? 0 : t2;
        if (v->tone1 == t1 && v->tone2 == want2)
            return v;
    }
    return NULL;
}

const EmojiToneVariant *skin_tone_uniform(int idx, int tone) {
    return skin_tone_pair(idx, tone, tone);
}

int skin_tone_has_pairs(int idx) {
    if (idx < 0 || idx >= EMOJI_COUNT)
        return 0;
    const EmojiEntry *e = &emoji_entries[idx];
    for (int i = 0; i < e->tones_count; i++) {
        if (emoji_tone_variants[e->tones_start + i].tone2 != 0)
            return 1;
    }
    return 0;
}

const char *skin_tone_display_glyph(int idx) {
    if (g_tone > SKIN_TONE_DEFAULT) {
        const EmojiToneVariant *v = skin_tone_uniform(idx, g_tone);
        if (v)
            return v->glyph;
    }
    return emoji_entries[idx].glyph;
}
