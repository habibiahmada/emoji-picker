#include "model/emoji_model.h"

#include "model/history.h"
#include "model/search_query.h"
#include "model/skin_tone.h"

#include <stdlib.h>
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

static uint16_t g_max_version = UINT16_MAX;

void emoji_model_set_max_version(uint16_t version) {
    g_max_version = version;
}

int emoji_model_version_supported(uint16_t version) {
    return version <= g_max_version;
}

static int entry_supported(int idx) {
    return emoji_entries[idx].version <= g_max_version;
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
        p->hits[p->n_hits] = idx;
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
    return skin_tone_display_glyph(p->hits[i]);
}

const char *emoji_model_hit_name(EmojiPicker *p, int i) {
    if (p->hits[i] >= EMOJI_COUNT)
        return NULL;
    return emoji_entries[p->hits[i]].name;
}

int emoji_model_hit_tone_base(EmojiPicker *p, int i) {
    /* Exact glyphs (History) insert as-is; only grid/search cells follow tone. */
    if (p->hit_glyphs && p->hit_glyphs[i])
        return -1;
    uint16_t idx = p->hits[i];
    if (idx >= EMOJI_COUNT || emoji_entries[idx].tones_count == 0)
        return -1;
    return idx;
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

/* Split on space / - / _ like the query tokenizer. */
static void index_add_text(GHashTable *ht, const char *text, uint16_t idx) {
    char buf[128];
    size_t n = 0;
    for (const char *s = text;; s++) {
        char c = *s;
        if (c && c != ' ' && c != '-' && c != '_') {
            if (n + 1 < sizeof(buf))
                buf[n++] = c;
            continue;
        }
        if (n > 0) {
            buf[n] = 0;
            index_add_word(ht, buf, idx);
            n = 0;
        }
        if (!c)
            break;
    }
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
        for (int t = 0; t < emoji_entries[i].tones_count; t++)
            g_hash_table_insert(
                p->glyph_index,
                (gpointer)emoji_tone_variants[emoji_entries[i].tones_start + t].glyph,
                GUINT_TO_POINTER((guint)i));
        index_add_text(p->word_index, emoji_entries[i].name, (uint16_t)i);
        index_add_text(p->word_index, emoji_entries[i].keywords, (uint16_t)i);
    }
    p->index_ready = TRUE;
}

static void collect_category(EmojiPicker *p, int category) {
    emoji_model_hits_reset(p);
    if (category < 0 || category >= EMOJI_CATEGORY_COUNT)
        return;
    EmojiCategoryRange r = emoji_category_ranges[category];
    for (uint16_t i = 0; i < r.count; i++) {
        if (entry_supported(r.start + i))
            emoji_model_hits_push(p, (uint16_t)(r.start + i), NULL);
    }
}

static void collect_all(EmojiPicker *p) {
    emoji_model_hits_reset(p);
    for (int i = 0; i < EMOJI_COUNT; i++) {
        if (entry_supported(i))
            emoji_model_hits_push(p, (uint16_t)i, NULL);
    }
}

static void collect_history(EmojiPicker *p) {
    emoji_model_hits_reset(p);
    emoji_model_build_indexes(p);
    int n = emoji_history_count();
    for (int i = 0; i < n; i++) {
        const char *g = emoji_history_get(i);
        /* Keep the exact glyph picked (tone included); index gives the name. */
        gpointer val = NULL;
        gboolean known = g_hash_table_lookup_extended(p->glyph_index, g, NULL, &val);
        emoji_model_hits_push(p, known ? (uint16_t)GPOINTER_TO_UINT(val) : EMOJI_NO_INDEX,
                              g);
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

typedef struct {
    uint16_t idx;
    int score;
} ScoredHit;

/* Best score first; ties keep Unicode order. */
static int scored_hit_cmp(const void *a, const void *b) {
    const ScoredHit *x = a, *y = b;
    if (x->score != y->score)
        return y->score - x->score;
    return (x->idx > y->idx) - (x->idx < y->idx);
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
        if (_i < EMOJI_COUNT && !seen[_i] && entry_supported(_i)) {                                   \
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

        /* Every word must match the name or keywords; name matches rank
         * first, so "stone" lists 🥌 💎 before 🗿 (keyword only). */
        ScoredHit *scored = g_new(ScoredHit, cands->len ? cands->len : 1);
        guint n_scored = 0;
        for (guint i = 0; i < cands->len; i++) {
            uint16_t idx = g_array_index(cands, uint16_t, i);
            int total = 0;
            for (int t = 0; t < ntok; t++) {
                int s = search_token_score(emoji_entries[idx].name,
                                           emoji_entries[idx].keywords, words[t]);
                if (!s) {
                    total = 0;
                    break;
                }
                total += s;
            }
            if (total)
                scored[n_scored++] = (ScoredHit){idx, total};
        }
        qsort(scored, n_scored, sizeof(ScoredHit), scored_hit_cmp);
        for (guint i = 0; i < n_scored; i++)
            ADD_HIT(scored[i].idx);
        g_free(scored);
        g_array_free(cands, TRUE);
    }

#undef ADD_HIT
    g_free(seen);
}
