#include "ui/picker_grid.h"

#include "model/emoji_model.h"
#include "ui/grid_layout.h"
#include "ui/picker_tone.h"

static GtkCssProvider *g_btn_css;
static GtkCssProvider *g_tab_css;

void picker_css_ensure(void) {
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
            "button.emoji-cell.emoji-tone-current {"
            "  background: alpha(@theme_selected_bg_color, 0.55);"
            "}"
            "button.tone-swatch {"
            "  min-width: 18px;"
            "  min-height: 18px;"
            "  padding: 0;"
            "  border-radius: 50%;"
            "  border: 1px solid alpha(@theme_fg_color, 0.25);"
            "}"
            "button.tone-swatch.active {"
            "  box-shadow: 0 0 0 2px @theme_selected_bg_color;"
            "}"
            "button.tone-1 { background: #f7d7c4; }"
            "button.tone-2 { background: #d8b094; }"
            "button.tone-3 { background: #bb9167; }"
            "button.tone-4 { background: #8e562e; }"
            "button.tone-5 { background: #613d30; }"
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

GtkCssProvider *picker_css_buttons(void) {
    picker_css_ensure();
    return g_btn_css;
}

GtkCssProvider *picker_css_tabs(void) {
    picker_css_ensure();
    return g_tab_css;
}

/*
 * Virtual grid: a fixed pool of buttons covering the visible rows lives in a
 * GtkLayout sized to every row. Scrolling moves and relabels the pool, so the
 * cost per frame stays the same for 80 or 1900 hits (a FlowBox holding every
 * hit re-laid out all of them on each page load).
 */

static void on_widget_realize_hand(GtkWidget *w, gpointer user_data) {
    (void)user_data;
    GdkWindow *win = gtk_widget_get_window(w);
    if (!win)
        return;
    GdkCursor *cur =
        gdk_cursor_new_for_display(gdk_window_get_display(win), GDK_HAND2);
    gdk_window_set_cursor(win, cur);
    g_object_unref(cur);
}

/* Right-click a toned cell: change skin tone even after one was saved. */
static gboolean on_cell_button_press(GtkWidget *btn, GdkEventButton *ev,
                                     gpointer user_data) {
    if (ev->type != GDK_BUTTON_PRESS || ev->button != GDK_BUTTON_SECONDARY)
        return FALSE;
    int base = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(btn), "tone-base")) - 1;
    if (base < 0)
        return FALSE;
    picker_tone_popover_show(user_data, btn, base);
    return TRUE;
}

static GtkWidget *make_pool_button(EmojiPicker *p) {
    GtkWidget *btn = gtk_button_new_with_label("");
    gtk_widget_set_size_request(btn, 40, 40);
    gtk_widget_set_can_focus(btn, FALSE);
    gtk_widget_set_focus_on_click(btn, FALSE);
    gtk_style_context_add_class(gtk_widget_get_style_context(btn), "emoji-cell");
    gtk_style_context_add_provider(gtk_widget_get_style_context(btn),
                                   GTK_STYLE_PROVIDER(g_btn_css),
                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_signal_connect(btn, "clicked", G_CALLBACK(picker_action_emoji_clicked), p);
    g_signal_connect(btn, "button-press-event", G_CALLBACK(on_cell_button_press), p);
    g_signal_connect(btn, "realize", G_CALLBACK(on_widget_realize_hand), NULL);
    gtk_widget_set_no_show_all(btn, TRUE);
    gtk_layout_put(GTK_LAYOUT(p->grid), btn, 0, 0);
    g_object_set_data(G_OBJECT(btn), "hit", GINT_TO_POINTER(-1));
    return btn;
}

static void ensure_pool(EmojiPicker *p, int need) {
    if (need <= p->pool_cap)
        return;
    p->pool = g_realloc(p->pool, (gsize)need * sizeof(GtkWidget *));
    for (int i = p->pool_cap; i < need; i++)
        p->pool[i] = make_pool_button(p);
    p->pool_cap = need;
}

static void set_cell(EmojiPicker *p, GtkWidget *btn, int i) {
    const char *glyph = emoji_model_hit_glyph(p, i);
    const char *name = emoji_model_hit_name(p, i);
    gtk_button_set_label(GTK_BUTTON(btn), glyph);
    g_object_set_data(G_OBJECT(btn), "glyph", (gpointer)glyph);
    g_object_set_data(G_OBJECT(btn), "tone-base",
                      GINT_TO_POINTER(emoji_model_hit_tone_base(p, i) + 1));
    g_object_set_data(G_OBJECT(btn), "hit", GINT_TO_POINTER(i));
    gtk_widget_set_tooltip_text(btn, name ? name : glyph);
}

static GtkAdjustment *grid_vadj(EmojiPicker *p) {
    return gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(p->grid));
}

/* force: hits or glyphs changed, so relabel even if the row did not move. */
static void render(EmojiPicker *p, gboolean force) {
    if (p->cell_h <= 0 || p->cell_w <= 0)
        return;
    double scroll_y = gtk_adjustment_get_value(grid_vadj(p));
    int first = grid_first_row(scroll_y, p->cell_h);
    if (!force && first == p->rendered_first)
        return;
    p->rendered_first = first;

    /* Ring buffer: hit i always uses pool[grid_slot(i)], so scrolling one row
     * relabels only that row; the rest just move. */
    int start = grid_first_hit(scroll_y, p->cell_h);
    for (int i = start; i < start + p->pool_cap; i++) {
        GtkWidget *btn = p->pool[grid_slot(i, p->pool_cap)];
        if (i >= p->n_hits) {
            gtk_widget_hide(btn);
            g_object_set_data(G_OBJECT(btn), "hit", GINT_TO_POINTER(-1));
            continue;
        }
        if (force || GPOINTER_TO_INT(g_object_get_data(G_OBJECT(btn), "hit")) != i)
            set_cell(p, btn, i);
        int x = 0, y = 0;
        grid_cell_xy(i, p->cell_w, p->cell_h, &x, &y);
        gtk_layout_move(GTK_LAYOUT(p->grid), btn, x, y);
        gtk_widget_show(btn);
    }
}

static void update_extent(EmojiPicker *p) {
    if (p->cell_h <= 0)
        return;
    int rows = grid_rows(p->n_hits);
    /* Width 1: GtkLayout requests its own width, and tracking the allocation
     * here made the scrollbar flip on/off (400 <-> 387px) in a resize loop. */
    gtk_layout_set_size(GTK_LAYOUT(p->grid), 1, (guint)(rows * p->cell_h));
}

static gboolean relayout_idle(gpointer user_data) {
    EmojiPicker *p = user_data;
    p->relayout_idle = 0;
    gboolean first = p->cell_h <= 0;
    if (first) {
        int h = 0;
        gtk_widget_get_preferred_height(p->pool[0], NULL, &h);
        p->cell_h = grid_cell_height(h);
    }
    /* Scrolled-window width, not the grid's: see grid_cell_width(). */
    int cell_w = grid_cell_width(gtk_widget_get_allocated_width(p->scroll));
    int need = grid_pool_size(p->alloc_h, p->cell_h);
    if (!first && cell_w == p->cell_w && need <= p->pool_cap)
        return G_SOURCE_REMOVE;
    p->cell_w = cell_w;
    ensure_pool(p, need);
    update_extent(p);
    render(p, TRUE);
    return G_SOURCE_REMOVE;
}

/* Only note the new size here: showing/moving children from inside the
 * layout's own allocation left them unallocated and undrawn. */
static void on_grid_size_allocate(GtkWidget *w, GdkRectangle *a, gpointer user_data) {
    (void)w;
    EmojiPicker *p = user_data;
    if (a->width == p->alloc_w && a->height == p->alloc_h)
        return;
    p->alloc_w = a->width;
    p->alloc_h = a->height;
    if (!p->relayout_idle)
        p->relayout_idle = g_idle_add(relayout_idle, p);
}

GtkWidget *picker_grid_new(EmojiPicker *p) {
    picker_css_ensure();
    p->grid = gtk_layout_new(NULL, NULL);
    gtk_widget_set_can_focus(p->grid, FALSE);
    gtk_widget_set_hexpand(p->grid, TRUE);
    p->rendered_first = -1;
    /* First page before the first allocation; size-allocate tops it up. */
    ensure_pool(p, 7 * GRID_COLS);
    g_signal_connect(p->grid, "size-allocate", G_CALLBACK(on_grid_size_allocate), p);
    return p->grid;
}

void picker_grid_resync_visibility(EmojiPicker *p) {
    render(p, TRUE);
}

void picker_grid_refresh_labels(EmojiPicker *p) {
    render(p, TRUE);
}

void picker_grid_present_reset(EmojiPicker *p) {
    gtk_adjustment_set_value(grid_vadj(p), 0);
    update_extent(p);
    render(p, TRUE);
}

void picker_grid_on_vadj_changed(GtkAdjustment *adj, gpointer user_data) {
    (void)adj;
    render(user_data, FALSE);
}
