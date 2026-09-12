#include "model/emoji_model.h"

#include "model/history.h"
#include "model/search_query.h"

#include <string.h>

void emoji_model_hits_reset(EmojiPicker *p) {
    if (p->hit_glyphs) {
        for (int i = 0; i < p->n_hits; i++)
            g_free(p->hit_glyphs[i]);
        g_free(p->hit_glyphs);
        p->hit_glyphs = NULL;
    }
    p->n_hits = 0;
}

void emoji_model_hits_push(EmojiPicker *p, uint16_t idx, const char *orphan_glyph) {
    if (p->n_hits >= p->hits_cap) {
        int nc = p->hits_cap ? p->hits_cap * 2 : 256;
        p->hits = g_realloc(p->hits, (gsize)nc * sizeof(uint16_t));
        if (p->hit_glyphs)
            p->hit_glyphs = g_realloc(p->hit_glyphs, (gsize)nc * sizeof(char *));
        p->hits_cap = nc;
    }
    if (orphan_glyph) {
        if (!p->hit_glyphs)
            p->hit_glyphs = g_malloc0((gsize)p->hits_cap * sizeof(char *));
        p->hit_glyphs[p->n_hits] = g_strdup(orphan_glyph);
        p->hits[p->n_hits] = 0;
    } else {
        if (p->hit_glyphs)
            p->hit_glyphs[p->n_hits] = NULL;
        p->hits[p->n_hits] = idx;
    }
    p->n_hits++;
}

const char *emoji_model_hit_glyph(EmojiPicker *p, int i) {
    if (p->hit_glyphs && p->hit_glyphs[i])
        return p->hit_glyphs[i];
    return emoji_entries[p->hits[i]].glyph;
}

const char *emoji_model_hit_name(EmojiPicker *p, int i) {
    if (p->hit_glyphs && p->hit_glyphs[i])
        return NULL;
    return emoji_entries[p->hits[i]].name;
}

static void index_add_word(GHashTable *ht, const char *word, uint16_t idx) {
    GArray *arr = g_hash_table_lookup(ht, word);
    if (!arr) {
        arr = g_array_new(FALSE, FALSE, sizeof(uint16_t));
        g_hash_table_insert(ht, g_strdup(word), arr);
    } else if (arr->len > 0 && g_array_index(arr, uint16_t, arr->len - 1) == idx)
        return;
    g_array_append_val(arr, idx);
}

static void free_index_array(gpointer data) {
    g_array_free((GArray *)data, TRUE);
}

void emoji_model_build_indexes(EmojiPicker *p) {
    if (p->index_ready)
        return;
    p->word_index = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
                                          free_index_array);
    p->glyph_index = g_hash_table_new(g_str_hash, g_str_equal);
    for (int i = 0; i < EMOJI_COUNT; i++) {
        g_hash_table_insert(p->glyph_index, (gpointer)emoji_entries[i].glyph,
                            GUINT_TO_POINTER((guint)i));
        const char *name = emoji_entries[i].name;
        char buf[128];
        size_t n = 0;
        for (const char *s = name;; s++) {
            char c = *s;
            if (c && c != ' ' && c != '-' && c != '_') {
                if (n + 1 < sizeof(buf))
                    buf[n++] = c;
                continue;
            }
            if (n > 0) {
                buf[n] = 0;
                index_add_word(p->word_index, buf, (uint16_t)i);
                n = 0;
            }
            if (!c)
                break;
        }
    }
    p->index_ready = TRUE;
}

static void collect_category(EmojiPicker *p, int category) {
    emoji_model_hits_reset(p);
    if (category < 0 || category >= EMOJI_CATEGORY_COUNT)
        return;
    EmojiCategoryRange r = emoji_category_ranges[category];
    for (uint16_t i = 0; i < r.count; i++)
        emoji_model_hits_push(p, (uint16_t)(r.start + i), NULL);
}

static void collect_all(EmojiPicker *p) {
    emoji_model_hits_reset(p);
    for (int i = 0; i < EMOJI_COUNT; i++)
        emoji_model_hits_push(p, (uint16_t)i, NULL);
}

static void collect_history(EmojiPicker *p) {
    emoji_model_hits_reset(p);
    emoji_model_build_indexes(p);
    int n = emoji_history_count();
    for (int i = 0; i < n; i++) {
        const char *g = emoji_history_get(i);
        gpointer val = g_hash_table_lookup(p->glyph_index, g);
        if (val)
            emoji_model_hits_push(p, (uint16_t)GPOINTER_TO_UINT(val), NULL);
        else
            emoji_model_hits_push(p, 0, g);
    }
}

void emoji_model_collect_for_tab(EmojiPicker *p, int tab) {
    if (tab == TAB_HISTORY)
        collect_history(p);
    else if (tab == TAB_ALL)
        collect_all(p);
    else
        collect_category(p, tab - TAB_CAT_BASE);
}

static void candidates_for_token(EmojiPicker *p, const char *tok, GArray *out) {
    GHashTableIter iter;
    gpointer key, value;
    g_hash_table_iter_init(&iter, p->word_index);
    size_t tlen = strlen(tok);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        const char *word = key;
        if (strncmp(word, tok, tlen) != 0)
            continue;
        GArray *arr = value;
        for (guint i = 0; i < arr->len; i++)
            g_array_append_val(out, g_array_index(arr, uint16_t, i));
    }
}

static int uint16_cmp(const void *a, const void *b) {
    uint16_t x = *(const uint16_t *)a, y = *(const uint16_t *)b;
    return (x > y) - (x < y);
}

static void unique_sorted(GArray *arr) {
    if (arr->len < 2)
        return;
    g_array_sort(arr, uint16_cmp);
    guint w = 1;
    for (guint i = 1; i < arr->len; i++) {
        if (g_array_index(arr, uint16_t, i) !=
            g_array_index(arr, uint16_t, w - 1)) {
            g_array_index(arr, uint16_t, w) = g_array_index(arr, uint16_t, i);
            w++;
        }
    }
    g_array_set_size(arr, w);
}

void emoji_model_collect_search(EmojiPicker *p, const char *query) {
    emoji_model_hits_reset(p);
    emoji_model_build_indexes(p);

    if (!query || !query[0])
        return;

    guint8 *seen = g_malloc0(EMOJI_COUNT);
#define ADD_HIT(idx)                                                           \
    do {                                                                       \
        uint16_t _i = (uint16_t)(idx);                                         \
        if (_i < EMOJI_COUNT && !seen[_i]) {                                   \
            seen[_i] = 1;                                                      \
            emoji_model_hits_push(p, _i, NULL);                                \
        }                                                                      \
    } while (0)

    gpointer gval = g_hash_table_lookup(p->glyph_index, query);
    if (gval)
        ADD_HIT(GPOINTER_TO_UINT(gval));

    for (int i = 0; i < EMOJI_COUNT; i++) {
        if (strstr(emoji_entries[i].glyph, query))
            ADD_HIT(i);
    }

    char tokens[EMOJI_SEARCH_TOKEN_MAX][EMOJI_SEARCH_TOKEN_LEN];
    int raw_ntok = search_tokenize_query(query, tokens, EMOJI_SEARCH_TOKEN_MAX);
    char words[EMOJI_SEARCH_TOKEN_MAX][EMOJI_SEARCH_TOKEN_LEN];
    int ntok = 0;
    for (int t = 0; t < raw_ntok; t++) {
        if (search_token_is_ascii_word(tokens[t]) && ntok < EMOJI_SEARCH_TOKEN_MAX) {
            memcpy(words[ntok], tokens[t], EMOJI_SEARCH_TOKEN_LEN);
            ntok++;
        }
    }

    if (ntok > 0) {
        int seed = 0;
        for (int i = 1; i < ntok; i++) {
            if (strlen(words[i]) > strlen(words[seed]))
                seed = i;
        }

        GArray *cands = g_array_new(FALSE, FALSE, sizeof(uint16_t));
        candidates_for_token(p, words[seed], cands);
        unique_sorted(cands);

        for (guint i = 0; i < cands->len; i++) {
            uint16_t idx = g_array_index(cands, uint16_t, i);
            if (search_name_has_all_tokens(emoji_entries[idx].name, words, ntok))
                ADD_HIT(idx);
        }
        g_array_free(cands, TRUE);
    }

#undef ADD_HIT
    g_free(seen);
}
