#include "popup.h"
#include "insert.h"
#include "history.h"
#include "generated/emoji_data.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gdk/gdkx.h>
#include <X11/Xlib.h>

#define PAGE_SIZE 80
#define FILL_CHUNK 40
#define SEARCH_DEBOUNCE_MS 60

/* Notebook pages: 0=History, 1=All, 2..=categories */
#define TAB_HISTORY 0
#define TAB_ALL 1
#define TAB_CAT_BASE 2
#define TAB_COUNT (TAB_CAT_BASE + EMOJI_CATEGORY_COUNT)

static const char *const tab_icon_names[TAB_COUNT] = {
    "emoji-recent-symbolic",          /* History */
    "view-grid-symbolic",             /* All */
    "face-smile-symbolic",            /* Smileys & Emotion */
    "emoji-people-symbolic",          /* People & Body */
    "emoji-nature-symbolic",          /* Animals & Nature */
    "emoji-food-symbolic",            /* Food & Drink */
    "emoji-travel-symbolic",          /* Travel & Places */
    "emoji-activities-symbolic",      /* Activities */
    "emoji-objects-symbolic",         /* Objects */
    "emoji-symbols-symbolic",         /* Symbols */
    "emoji-flags-symbolic",           /* Flags */
};

static const char *const tab_tips[TAB_COUNT] = {
    "History",
    "All emoji",
    "Smileys & Emotion",
    "People & Body",
    "Animals & Nature",
    "Food & Drink",
    "Travel & Places",
    "Activities",
    "Objects",
    "Symbols",
    "Flags",
};

struct EmojiPicker {
    GtkWidget *win;
    GtkWidget *search;
    GtkWidget *notebook;
    GtkWidget *scroll;
    GtkWidget *flow;

    GtkWidget **pool;
    int pool_cap;
    int shown;       /* buttons currently visible */
    int reveal_to;   /* how far into hits[] we have revealed (pagination) */

    uint16_t *hits;
    int n_hits;
    int hits_cap;

    /* For history rows that are not in emoji_entries (shouldn't happen often). */
    char **hit_glyphs; /* parallel to hits; NULL → use emoji_entries[hits[i]] */

    guint search_timeout;
    guint fill_idle;
    int fill_pos;
    gboolean loading_more;

    GHashTable *word_index;
    GHashTable *glyph_index; /* glyph → uint16 index */
    gboolean index_ready;

    gboolean suppress_hide;
    guint focus_out_timeout;
    guint reclaim_timeout;
};

static GtkCssProvider *g_btn_css;
static GtkCssProvider *g_tab_css;

static void ensure_css(void) {
    if (!g_btn_css) {
        g_btn_css = gtk_css_provider_new();
        gtk_css_provider_load_from_data(
            g_btn_css,
            "button.emoji-cell {"
            "  font-family: \"Noto Color Emoji\", \"Segoe UI Emoji\", sans-serif;"
            "  font-size: 20px;"
            "  padding: 0;"
            "  margin: 0;"
            "  min-width: 40px;"
            "  min-height: 40px;"
            "  max-width: 40px;"
            "  max-height: 40px;"
            "}"
            "button.emoji-cell:hover {"
            "  background: alpha(@theme_selected_bg_color, 0.35);"
            "}"
            "flowbox {"
            "  min-height: 0;"
            "}"
            "flowboxchild {"
            "  padding: 2px;"
            "  margin: 0;"
            "  min-width: 0;"
            "  min-height: 0;"
            "  outline: none;"
            "  border: none;"
            "  box-shadow: none;"
            "  background: transparent;"
            "}"
            "flowboxchild:focus,"
            "flowboxchild:selected,"
            "flowboxchild:hover {"
            "  outline: none;"
            "  border: none;"
            "  box-shadow: none;"
            "  background: transparent;"
            "}"
            "entry.emoji-search,"
            "entry.emoji-search text {"
            "  font-family: sans-serif;"
            "  font-size: 13px;"
            "  caret-color: @theme_fg_color;"
            "  min-height: 1.2em;"
            "}"
            "entry.emoji-search {"
            "  min-height: 32px;"
            "  -GtkWidget-cursor-aspect-ratio: 0.04;"
            "}",
            -1, NULL);
    }
    if (!g_tab_css) {
        g_tab_css = gtk_css_provider_new();
        gtk_css_provider_load_from_data(
            g_tab_css,
            "notebook tab {"
            "  padding: 6px 8px;"
            "  min-width: 0;"
            "  outline: none;"
            "  border: none;"
            "  box-shadow: none;"
            "}"
            "notebook tab:checked,"
            "notebook tab:hover,"
            "notebook tab:focus {"
            "  outline: none;"
            "  box-shadow: none;"
            "}"
            "notebook tab:checked {"
            "  background: alpha(@theme_selected_bg_color, 0.35);"
            "  border-radius: 4px;"
            "}"
            "notebook tab image { -gtk-icon-style: symbolic; }",
            -1, NULL);
    }
}

static void set_hand_cursor(GtkWidget *w) {
    GdkWindow *win = gtk_widget_get_window(w);
    if (!win)
        return;
    GdkCursor *cur =
        gdk_cursor_new_for_display(gdk_window_get_display(win), GDK_HAND2);
    gdk_window_set_cursor(win, cur);
    g_object_unref(cur);
}

static void clear_cursor(GtkWidget *w) {
    GdkWindow *win = gtk_widget_get_window(w);
    if (win)
        gdk_window_set_cursor(win, NULL);
}

static void on_widget_realize_hand(GtkWidget *w, gpointer user_data) {
    (void)user_data;
    set_hand_cursor(w);
}

/* GtkFlowBox wraps children; hiding only the button leaves an empty selectable shell. */
static GtkWidget *flow_wrap(GtkWidget *btn) {
    GtkWidget *parent = gtk_widget_get_parent(btn);
    if (parent && GTK_IS_FLOW_BOX_CHILD(parent))
        return parent;
    return btn;
}

static void emoji_btn_show(GtkWidget *btn) {
    GtkWidget *wrap = flow_wrap(btn);
    gtk_widget_set_can_focus(btn, FALSE);
    gtk_widget_set_can_focus(wrap, FALSE);
    gtk_widget_show(btn);
    gtk_widget_show(wrap);
    if (gtk_widget_get_realized(btn))
        set_hand_cursor(btn);
}

static void emoji_btn_hide(GtkWidget *btn) {
    GtkWidget *wrap = flow_wrap(btn);
    clear_cursor(btn);
    gtk_widget_hide(btn);
    gtk_widget_hide(wrap);
}

static const char *hit_glyph(EmojiPicker *p, int i) {
    if (p->hit_glyphs && p->hit_glyphs[i])
        return p->hit_glyphs[i];
    return emoji_entries[p->hits[i]].glyph;
}

static const char *hit_name(EmojiPicker *p, int i) {
    if (p->hit_glyphs && p->hit_glyphs[i])
        return NULL;
    return emoji_entries[p->hits[i]].name;
}

static gboolean reclaim_focus_cb(gpointer user_data) {
    EmojiPicker *p = user_data;
    p->reclaim_timeout = 0;
    p->suppress_hide = FALSE;
    if (!gtk_widget_get_visible(p->win))
        return G_SOURCE_REMOVE;
    gtk_window_present(GTK_WINDOW(p->win));
    gtk_widget_grab_focus(p->search);
    return G_SOURCE_REMOVE;
}

static void collect_for_tab(EmojiPicker *p, int tab);
static void present_reset(EmojiPicker *p);
static void rebuild_visible(EmojiPicker *p);

static void on_emoji_clicked(GtkButton *btn, gpointer user_data) {
    EmojiPicker *p = user_data;
    const char *glyph = g_object_get_data(G_OBJECT(btn), "glyph");
    if (!glyph)
        return;

    p->suppress_hide = TRUE;
    if (p->focus_out_timeout) {
        g_source_remove(p->focus_out_timeout);
        p->focus_out_timeout = 0;
    }
    if (p->reclaim_timeout) {
        g_source_remove(p->reclaim_timeout);
        p->reclaim_timeout = 0;
    }

    emoji_history_push(glyph);
    emoji_insert(glyph);
    p->reclaim_timeout = g_timeout_add(180, reclaim_focus_cb, p);

    /* Refresh history tab contents if currently viewing it (no search). */
    const char *q = gtk_entry_get_text(GTK_ENTRY(p->search));
    int tab = gtk_notebook_get_current_page(GTK_NOTEBOOK(p->notebook));
    if ((!q || !q[0]) && tab == TAB_HISTORY) {
        collect_for_tab(p, TAB_HISTORY);
        present_reset(p);
    }
}

static GtkWidget *make_pool_button(EmojiPicker *p) {
    ensure_css();
    GtkWidget *btn = gtk_button_new_with_label("");
    gtk_widget_set_size_request(btn, 40, 40);
    gtk_widget_set_can_focus(btn, FALSE);
    gtk_widget_set_focus_on_click(btn, FALSE);
    gtk_style_context_add_class(gtk_widget_get_style_context(btn), "emoji-cell");
    gtk_style_context_add_provider(gtk_widget_get_style_context(btn),
                                   GTK_STYLE_PROVIDER(g_btn_css),
                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_signal_connect(btn, "clicked", G_CALLBACK(on_emoji_clicked), p);
    g_signal_connect(btn, "realize", G_CALLBACK(on_widget_realize_hand), NULL);
    gtk_widget_set_no_show_all(btn, TRUE);
    gtk_widget_hide(btn);
    gtk_container_add(GTK_CONTAINER(p->flow), btn);

    GtkWidget *wrap = flow_wrap(btn);
    if (wrap != btn) {
        gtk_widget_set_no_show_all(wrap, TRUE);
        gtk_widget_set_can_focus(wrap, FALSE);
        gtk_widget_set_hexpand(wrap, FALSE);
        gtk_widget_set_vexpand(wrap, FALSE);
        gtk_widget_set_halign(wrap, GTK_ALIGN_CENTER);
        gtk_widget_set_valign(wrap, GTK_ALIGN_START);
        gtk_widget_set_size_request(wrap, 44, 44);
        gtk_style_context_add_provider(gtk_widget_get_style_context(wrap),
                                       GTK_STYLE_PROVIDER(g_btn_css),
                                       GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        gtk_widget_hide(wrap);
    }
    return btn;
}

static void ensure_pool_size(EmojiPicker *p, int need) {
    if (need <= p->pool_cap)
        return;
    int nc = p->pool_cap ? p->pool_cap : PAGE_SIZE;
    while (nc < need)
        nc *= 2;
    p->pool = g_realloc(p->pool, (gsize)nc * sizeof(GtkWidget *));
    for (int i = p->pool_cap; i < nc; i++)
        p->pool[i] = make_pool_button(p);
    p->pool_cap = nc;
}

static void hide_unused_buttons(EmojiPicker *p, int from) {
    for (int i = from; i < p->shown; i++)
        emoji_btn_hide(p->pool[i]);
    /* Also hide any extra pool slots beyond shown (e.g. after show_all). */
    for (int i = p->shown; i < p->pool_cap; i++)
        emoji_btn_hide(p->pool[i]);
    if (from < p->shown)
        p->shown = from;
}

static gboolean fill_idle_cb(gpointer user_data) {
    EmojiPicker *p = user_data;
    int limit = p->reveal_to;
    int end = p->fill_pos + FILL_CHUNK;
    if (end > limit)
        end = limit;

    ensure_pool_size(p, end);
    for (int i = p->fill_pos; i < end; i++) {
        const char *glyph = hit_glyph(p, i);
        const char *name = hit_name(p, i);
        GtkWidget *btn = p->pool[i];
        gtk_button_set_label(GTK_BUTTON(btn), glyph);
        g_object_set_data(G_OBJECT(btn), "glyph", (gpointer)glyph);
        gtk_widget_set_tooltip_text(btn, name ? name : "");
        emoji_btn_show(btn);
    }
    p->fill_pos = end;
    if (end > p->shown)
        p->shown = end;

    if (p->fill_pos < limit)
        return G_SOURCE_CONTINUE;

    hide_unused_buttons(p, limit);
    p->fill_idle = 0;
    p->loading_more = FALSE;
    return G_SOURCE_REMOVE;
}

static void start_fill(EmojiPicker *p) {
    if (p->fill_idle) {
        g_source_remove(p->fill_idle);
        p->fill_idle = 0;
    }
    /* Paint first chunk now so UI responds immediately. */
    fill_idle_cb(p);
    if (p->fill_pos < p->reveal_to)
        p->fill_idle = g_idle_add(fill_idle_cb, p);
}

static void present_reset(EmojiPicker *p) {
    /* Drop old labels before hits_glyph pointers are invalidated by a new set. */
    hide_unused_buttons(p, 0);
    p->reveal_to = p->n_hits < PAGE_SIZE ? p->n_hits : PAGE_SIZE;
    p->fill_pos = 0;
    p->loading_more = FALSE;
    if (p->scroll) {
        GtkAdjustment *adj =
            gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(p->scroll));
        gtk_adjustment_set_value(adj, 0);
    }
    start_fill(p);
}

static void load_more(EmojiPicker *p) {
    if (p->loading_more || p->reveal_to >= p->n_hits)
        return;
    p->loading_more = TRUE;
    int next = p->reveal_to + PAGE_SIZE;
    if (next > p->n_hits)
        next = p->n_hits;
    p->fill_pos = p->reveal_to;
    p->reveal_to = next;
    start_fill(p);
}

static void on_vadj_changed(GtkAdjustment *adj, gpointer user_data) {
    EmojiPicker *p = user_data;
    double value = gtk_adjustment_get_value(adj);
    double upper = gtk_adjustment_get_upper(adj);
    double page = gtk_adjustment_get_page_size(adj);
    if (upper <= page)
        return;
    if (value + page >= upper - 64.0)
        load_more(p);
}

static void hits_reset(EmojiPicker *p) {
    if (p->hit_glyphs) {
        for (int i = 0; i < p->n_hits; i++)
            g_free(p->hit_glyphs[i]);
        g_free(p->hit_glyphs);
        p->hit_glyphs = NULL;
    }
    p->n_hits = 0;
}

static void hits_push(EmojiPicker *p, uint16_t idx, const char *orphan_glyph) {
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

static void collect_category(EmojiPicker *p, int category) {
    hits_reset(p);
    if (category < 0 || category >= EMOJI_CATEGORY_COUNT)
        return;
    EmojiCategoryRange r = emoji_category_ranges[category];
    for (uint16_t i = 0; i < r.count; i++)
        hits_push(p, (uint16_t)(r.start + i), NULL);
}

static void collect_all(EmojiPicker *p) {
    hits_reset(p);
    for (int i = 0; i < EMOJI_COUNT; i++)
        hits_push(p, (uint16_t)i, NULL);
}

static void build_indexes(EmojiPicker *p);

static void collect_history(EmojiPicker *p) {
    hits_reset(p);
    build_indexes(p);
    int n = emoji_history_count();
    for (int i = 0; i < n; i++) {
        const char *g = emoji_history_get(i);
        gpointer val = g_hash_table_lookup(p->glyph_index, g);
        if (val)
            hits_push(p, (uint16_t)GPOINTER_TO_UINT(val), NULL);
        else
            hits_push(p, 0, g);
    }
}

static void collect_for_tab(EmojiPicker *p, int tab) {
    if (tab == TAB_HISTORY)
        collect_history(p);
    else if (tab == TAB_ALL)
        collect_all(p);
    else
        collect_category(p, tab - TAB_CAT_BASE);
}

/* --- search index ------------------------------------------------------- */

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

static void build_indexes(EmojiPicker *p) {
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

static int tokenize_query(const char *q, char tokens[][64], int max_tok) {
    int ntok = 0;
    size_t n = 0;
    for (const char *s = q;; s++) {
        unsigned char c = (unsigned char)*s;
        if (c && !isspace(c) && c != '-' && c != '_') {
            if (ntok < max_tok && n + 1 < 64)
                tokens[ntok][n++] = (char)tolower(c);
            continue;
        }
        if (n > 0 && ntok < max_tok) {
            tokens[ntok][n] = 0;
            ntok++;
            n = 0;
        }
        if (!c)
            break;
    }
    return ntok;
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

static gboolean name_has_all_tokens(const char *name, char tokens[][64],
                                    int ntok) {
    for (int t = 0; t < ntok; t++) {
        if (!strstr(name, tokens[t]))
            return FALSE;
    }
    return TRUE;
}

static void collect_search(EmojiPicker *p, const char *query) {
    hits_reset(p);
    build_indexes(p);

    char tokens[8][64];
    int ntok = tokenize_query(query, tokens, 8);
    if (ntok == 0)
        return;

    int seed = 0;
    for (int i = 1; i < ntok; i++) {
        if (strlen(tokens[i]) > strlen(tokens[seed]))
            seed = i;
    }

    GArray *cands = g_array_new(FALSE, FALSE, sizeof(uint16_t));
    candidates_for_token(p, tokens[seed], cands);
    unique_sorted(cands);

    for (guint i = 0; i < cands->len; i++) {
        uint16_t idx = g_array_index(cands, uint16_t, i);
        if (name_has_all_tokens(emoji_entries[idx].name, tokens, ntok))
            hits_push(p, idx, NULL);
    }
    g_array_free(cands, TRUE);
}

static void rebuild_visible(EmojiPicker *p) {
    const char *q = gtk_entry_get_text(GTK_ENTRY(p->search));
    char query[256];
    size_t n = 0;
    for (const char *s = q; *s && n + 1 < sizeof(query); s++)
        query[n++] = (char)tolower((unsigned char)*s);
    query[n] = 0;

    if (query[0])
        collect_search(p, query);
    else {
        int tab = gtk_notebook_get_current_page(GTK_NOTEBOOK(p->notebook));
        if (tab < 0 || tab >= TAB_COUNT)
            tab = TAB_ALL;
        collect_for_tab(p, tab);
    }
    present_reset(p);
}

static gboolean search_timeout_cb(gpointer user_data) {
    EmojiPicker *p = user_data;
    p->search_timeout = 0;
    rebuild_visible(p);
    return G_SOURCE_REMOVE;
}

static void on_search_changed(GtkEditable *editable, gpointer user_data) {
    (void)editable;
    EmojiPicker *p = user_data;
    if (p->search_timeout)
        g_source_remove(p->search_timeout);
    p->search_timeout = g_timeout_add(SEARCH_DEBOUNCE_MS, search_timeout_cb, p);
}

static void on_page_switched(GtkNotebook *nb, GtkWidget *page, guint page_num,
                             gpointer user_data) {
    (void)nb;
    (void)page;
    EmojiPicker *p = user_data;
    const char *q = gtk_entry_get_text(GTK_ENTRY(p->search));
    if (q && q[0])
        return;
    if ((int)page_num >= TAB_COUNT)
        return;
    collect_for_tab(p, (int)page_num);
    present_reset(p);
}

static gboolean on_key_press(GtkWidget *w, GdkEventKey *ev, gpointer user_data) {
    (void)w;
    EmojiPicker *p = user_data;
    if (ev->keyval == GDK_KEY_Escape) {
        gtk_widget_hide(p->win);
        return TRUE;
    }
    return FALSE;
}

static gboolean hide_if_inactive(gpointer data) {
    EmojiPicker *p = data;
    p->focus_out_timeout = 0;
    if (p->suppress_hide)
        return G_SOURCE_REMOVE;
    if (!gtk_widget_get_visible(p->win))
        return G_SOURCE_REMOVE;
    if (!gtk_window_is_active(GTK_WINDOW(p->win)))
        gtk_widget_hide(p->win);
    return G_SOURCE_REMOVE;
}

static gboolean on_focus_out_c(GtkWidget *w, GdkEventFocus *ev,
                               gpointer user_data) {
    (void)w;
    (void)ev;
    EmojiPicker *p = user_data;
    if (p->suppress_hide)
        return FALSE;
    if (p->focus_out_timeout)
        g_source_remove(p->focus_out_timeout);
    p->focus_out_timeout = g_timeout_add(200, hide_if_inactive, p);
    return FALSE;
}

static void apply_window_hints(GtkWidget *win) {
    gtk_window_set_decorated(GTK_WINDOW(win), FALSE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(win), TRUE);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(win), TRUE);
    gtk_window_set_keep_above(GTK_WINDOW(win), TRUE);
    gtk_window_set_type_hint(GTK_WINDOW(win), GDK_WINDOW_TYPE_HINT_UTILITY);
    gtk_window_set_accept_focus(GTK_WINDOW(win), TRUE);
    gtk_window_set_resizable(GTK_WINDOW(win), FALSE);
}

static void place_near_pointer(GtkWidget *win) {
    GdkDisplay *dpy = gdk_display_get_default();
    GdkSeat *seat = gdk_display_get_default_seat(dpy);
    GdkDevice *ptr = gdk_seat_get_pointer(seat);
    int x = 0, y = 0;
    gdk_device_get_position(ptr, NULL, &x, &y);

    int w = 420, h = 360;
    gtk_window_get_size(GTK_WINDOW(win), &w, &h);

    GdkMonitor *mon = gdk_display_get_monitor_at_point(dpy, x, y);
    GdkRectangle geo;
    gdk_monitor_get_workarea(mon, &geo);

    int px = x + 12;
    int py = y + 12;
    if (px + w > geo.x + geo.width)
        px = geo.x + geo.width - w - 8;
    if (py + h > geo.y + geo.height)
        py = geo.y + geo.height - h - 8;
    if (px < geo.x)
        px = geo.x + 8;
    if (py < geo.y)
        py = geo.y + 8;
    gtk_window_move(GTK_WINDOW(win), px, py);
}

static uint32_t capture_active_xid(void) {
    GdkDisplay *gdpy = gdk_display_get_default();
    if (!GDK_IS_X11_DISPLAY(gdpy))
        return 0;
    Display *dpy = GDK_DISPLAY_XDISPLAY(gdpy);
    Window focus = None;
    int revert = 0;
    XGetInputFocus(dpy, &focus, &revert);
    if (focus == None || focus == PointerRoot)
        return 0;
    return (uint32_t)focus;
}

static uint32_t widget_xid(GtkWidget *w) {
    GdkWindow *gw = gtk_widget_get_window(w);
    if (!gw || !GDK_IS_X11_WINDOW(gw))
        return 0;
    return (uint32_t)gdk_x11_window_get_xid(gw);
}

static GtkWidget *make_tab_label(int i) {
    GtkWidget *img =
        gtk_image_new_from_icon_name(tab_icon_names[i], GTK_ICON_SIZE_MENU);
    /* EventBox so tab gets hover cursor + reliable tooltip. */
    GtkWidget *box = gtk_event_box_new();
    gtk_widget_set_tooltip_text(box, tab_tips[i]);
    gtk_container_add(GTK_CONTAINER(box), img);
    g_signal_connect(box, "realize", G_CALLBACK(on_widget_realize_hand), NULL);
    gtk_widget_show_all(box);
    return box;
}

EmojiPicker *emoji_picker_new(void) {
    emoji_history_load();

    EmojiPicker *p = g_new0(EmojiPicker, 1);
    p->win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(p->win), "Emoji");
    gtk_window_set_default_size(GTK_WINDOW(p->win), 420, 360);
    apply_window_hints(p->win);
    ensure_css();

    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(root), 10);
    gtk_container_add(GTK_CONTAINER(p->win), root);

    p->search = gtk_entry_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(p->search),
                                "emoji-search");
    gtk_style_context_add_provider(gtk_widget_get_style_context(p->search),
                                   GTK_STYLE_PROVIDER(g_btn_css),
                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    gtk_entry_set_placeholder_text(GTK_ENTRY(p->search),
                                   "Search… smile, fire, cat");
    g_signal_connect(p->search, "changed", G_CALLBACK(on_search_changed), p);
    gtk_box_pack_start(GTK_BOX(root), p->search, FALSE, FALSE, 0);

    p->notebook = gtk_notebook_new();
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(p->notebook), TRUE);
    gtk_notebook_set_show_border(GTK_NOTEBOOK(p->notebook), FALSE);
    gtk_style_context_add_provider(gtk_widget_get_style_context(p->notebook),
                                   GTK_STYLE_PROVIDER(g_tab_css),
                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    gtk_box_pack_start(GTK_BOX(root), p->notebook, FALSE, FALSE, 0);

    for (int i = 0; i < TAB_COUNT; i++) {
        GtkWidget *stub = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_widget_set_size_request(stub, 0, 0);
        gtk_notebook_append_page(GTK_NOTEBOOK(p->notebook), stub,
                                 make_tab_label(i));
    }

    p->scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(p->scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(p->scroll, TRUE);
    gtk_box_pack_start(GTK_BOX(root), p->scroll, TRUE, TRUE, 0);

    GtkAdjustment *vadj =
        gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(p->scroll));
    g_signal_connect(vadj, "value-changed", G_CALLBACK(on_vadj_changed), p);

    p->flow = gtk_flow_box_new();
    gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(p->flow), 8);
    gtk_flow_box_set_min_children_per_line(GTK_FLOW_BOX(p->flow), 1);
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(p->flow), GTK_SELECTION_NONE);
    gtk_flow_box_set_homogeneous(GTK_FLOW_BOX(p->flow), TRUE);
    gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(p->flow), 2);
    gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(p->flow), 2);
    gtk_widget_set_can_focus(p->flow, FALSE);
    gtk_widget_set_hexpand(p->flow, TRUE);
    gtk_widget_set_vexpand(p->flow, FALSE);
    gtk_widget_set_valign(p->flow, GTK_ALIGN_START);
    gtk_widget_set_halign(p->flow, GTK_ALIGN_FILL);
    gtk_style_context_add_provider(gtk_widget_get_style_context(p->flow),
                                   GTK_STYLE_PROVIDER(g_btn_css),
                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    gtk_container_add(GTK_CONTAINER(p->scroll), p->flow);

    g_signal_connect(p->notebook, "switch-page", G_CALLBACK(on_page_switched), p);
    g_signal_connect(p->win, "key-press-event", G_CALLBACK(on_key_press), p);
    g_signal_connect(p->win, "focus-out-event", G_CALLBACK(on_focus_out_c), p);

    build_indexes(p);
    ensure_pool_size(p, PAGE_SIZE);

    int start = emoji_history_count() > 0 ? TAB_HISTORY : TAB_ALL;
    gtk_notebook_set_current_page(GTK_NOTEBOOK(p->notebook), start);
    collect_for_tab(p, start);
    present_reset(p);

    return p;
}

void emoji_picker_show(EmojiPicker *p) {
    uint32_t target = capture_active_xid();

    gtk_widget_show_all(p->win);
    /* show_all must not resurrect empty FlowBox shells — re-sync visibility. */
    for (int i = 0; i < p->pool_cap; i++) {
        if (i < p->shown)
            emoji_btn_show(p->pool[i]);
        else
            emoji_btn_hide(p->pool[i]);
    }

    place_near_pointer(p->win);
    gtk_window_present(GTK_WINDOW(p->win));
    gtk_widget_grab_focus(p->search);

    uint32_t picker = widget_xid(p->win);
    emoji_insert_set_picker(picker);
    if (target != 0 && target != picker)
        emoji_insert_set_target(target);

    const char *q = gtk_entry_get_text(GTK_ENTRY(p->search));
    if (q && q[0]) {
        g_signal_handlers_block_by_func(p->search, (gpointer)on_search_changed,
                                        p);
        gtk_entry_set_text(GTK_ENTRY(p->search), "");
        g_signal_handlers_unblock_by_func(p->search, (gpointer)on_search_changed,
                                          p);
    }

    int start = emoji_history_count() > 0 ? TAB_HISTORY : TAB_ALL;
    gtk_notebook_set_current_page(GTK_NOTEBOOK(p->notebook), start);
    collect_for_tab(p, start);
    present_reset(p);
}

GtkWidget *emoji_picker_window(EmojiPicker *p) {
    return p->win;
}
