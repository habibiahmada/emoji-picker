#include "model/emoji_model.h"
#include "ui/picker_priv.h"
#include "test_harness.h"

#include <string.h>

static void test_search_cat_finds_entries(void) {
    EmojiPicker *p = g_new0(EmojiPicker, 1);
    emoji_model_build_indexes(p);
    emoji_model_collect_search(p, "cat");
    TEST_ASSERT(p->n_hits > 0);

    int found_name = 0;
    for (int i = 0; i < p->n_hits; i++) {
        const char *name = emoji_model_hit_name(p, i);
        if (name && strstr(name, "cat")) {
            found_name = 1;
            break;
        }
    }
    TEST_ASSERT(found_name);

    emoji_model_hits_reset(p);
    if (p->word_index)
        g_hash_table_destroy(p->word_index);
    if (p->glyph_index)
        g_hash_table_destroy(p->glyph_index);
    g_free(p->hits);
    g_free(p);
}

static void free_picker(EmojiPicker *p) {
    emoji_model_hits_reset(p);
    if (p->word_index)
        g_hash_table_destroy(p->word_index);
    if (p->glyph_index)
        g_hash_table_destroy(p->glyph_index);
    g_free(p->hits);
    g_free(p);
}

/* Position of glyph in the hits of query, or -1. */
static int search_rank(const char *query, const char *glyph) {
    EmojiPicker *p = g_new0(EmojiPicker, 1);
    emoji_model_collect_search(p, query);
    int rank = -1;
    for (int i = 0; i < p->n_hits && rank < 0; i++) {
        if (strcmp(emoji_model_hit_glyph(p, i), glyph) == 0)
            rank = i;
    }
    free_picker(p);
    return rank;
}

static void test_search_keywords(void) {
    /* CLDR English, CLDR Indonesian, and the hand-kept extra file. */
    TEST_ASSERT(search_rank("statue", "\xf0\x9f\x97\xbf") >= 0);   /* moai */
    TEST_ASSERT(search_rank("ngakak", "\xf0\x9f\x98\x82") >= 0);   /* tears of joy */
    TEST_ASSERT(search_rank("rock", "\xf0\x9f\x97\xbf") >= 0);
    TEST_ASSERT(search_rank("batu", "\xf0\x9f\x97\xbf") >= 0);
    TEST_ASSERT(search_rank("lol", "\xf0\x9f\x98\x82") >= 0);
}

static void test_search_name_ranks_first(void) {
    /* 🪨 is named "rock"; 🗿 only has it as a keyword. */
    int rock = search_rank("rock", "\xf0\x9f\xaa\xa8");
    int moai = search_rank("rock", "\xf0\x9f\x97\xbf");
    TEST_ASSERT(rock >= 0 && moai > rock);
    /* A whole keyword beats a name prefix: 🗿 before 🚀 "rocket". */
    TEST_ASSERT(moai < search_rank("rock", "\xf0\x9f\x9a\x80"));
    /* The emoji named exactly "fire" leads, not ❤️‍🔥 "heart on fire". */
    TEST_ASSERT(search_rank("fire", "\xf0\x9f\x94\xa5") == 0);
    /* Every word must match: "rock face" is not 🪨. */
    TEST_ASSERT(search_rank("rock face", "\xf0\x9f\xaa\xa8") < 0);
    TEST_ASSERT(search_rank("rock face", "\xf0\x9f\x97\xbf") >= 0);
}

static void test_collect_all_count(void) {
    EmojiPicker *p = g_new0(EmojiPicker, 1);
    emoji_model_collect_for_tab(p, TAB_ALL);
    TEST_ASSERT(p->n_hits == EMOJI_COUNT);
    emoji_model_hits_reset(p);
    g_free(p->hits);
    g_free(p);
}

static void test_max_version_hides_newer(void) {
    int newer = 0;
    for (int i = 0; i < EMOJI_COUNT; i++)
        newer += emoji_entries[i].version > 160;
    TEST_ASSERT(newer > 0);

    EmojiPicker *p = g_new0(EmojiPicker, 1);
    emoji_model_set_max_version(160);
    emoji_model_collect_for_tab(p, TAB_ALL);
    TEST_ASSERT(p->n_hits == EMOJI_COUNT - newer);
    emoji_model_set_max_version(UINT16_MAX);
    emoji_model_hits_reset(p);
    g_free(p->hits);
    g_free(p);
}

int main(void) {
    TEST_RUN(test_search_cat_finds_entries);
    TEST_RUN(test_search_keywords);
    TEST_RUN(test_search_name_ranks_first);
    TEST_RUN(test_collect_all_count);
    TEST_RUN(test_max_version_hides_newer);
    return test_finish();
}
