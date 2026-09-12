#ifndef PICKER_PRIV_H
#define PICKER_PRIV_H

#include "ui/popup.h"

#include <stdint.h>

#include <gtk/gtk.h>

#include "generated/emoji_data.h"

#define PAGE_SIZE 80
#define FILL_CHUNK 40
#define SEARCH_DEBOUNCE_MS 60
#define DISMISS_POLL_MS 50

#define TAB_HISTORY 0
#define TAB_ALL 1
#define TAB_CAT_BASE 2
#define TAB_COUNT (TAB_CAT_BASE + EMOJI_CATEGORY_COUNT)

struct EmojiPicker {
    GtkWidget *win;
    GtkWidget *search;
    GtkWidget *notebook;
    GtkWidget *scroll;
    GtkWidget *flow;

    GtkWidget **pool;
    int pool_cap;
    int shown;
    int reveal_to;

    uint16_t *hits;
    int n_hits;
    int hits_cap;
    char **hit_glyphs;

    guint search_timeout;
    guint fill_idle;
    int fill_pos;
    gboolean loading_more;

    GHashTable *word_index;
    GHashTable *glyph_index;
    gboolean index_ready;

    gboolean suppress_hide;
    guint focus_out_timeout;
    guint status_timeout;
    guint finish_insert_timeout;
    guint dismiss_poll;
    gint64 last_insert_us;
    uint32_t sticky_target;
    char prev_keymap[32];
    gboolean keymap_ready;
    gboolean outside_btn_latched;

    GtkWidget *status;

    int pos_x;
    int pos_y;
    gboolean pos_valid;
};

#endif
