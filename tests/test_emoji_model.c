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

static void test_collect_all_count(void) {
    EmojiPicker *p = g_new0(EmojiPicker, 1);
    emoji_model_collect_for_tab(p, TAB_ALL);
    TEST_ASSERT(p->n_hits == EMOJI_COUNT);
    emoji_model_hits_reset(p);
    g_free(p->hits);
    g_free(p);
}

int main(void) {
    TEST_RUN(test_search_cat_finds_entries);
    TEST_RUN(test_collect_all_count);
    return test_finish();
}
