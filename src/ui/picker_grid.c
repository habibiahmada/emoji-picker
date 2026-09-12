#include "ui/picker_grid.h"

#include "model/emoji_model.h"

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

GtkCssProvider *picker_css_buttons(void) {
    picker_css_ensure();
    return g_btn_css;
}

GtkCssProvider *picker_css_tabs(void) {
    picker_css_ensure();
    return g_tab_css;
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

void picker_grid_resync_visibility(EmojiPicker *p) {
    for (int i = 0; i < p->pool_cap; i++) {
        if (i < p->shown)
            emoji_btn_show(p->pool[i]);
        else
            emoji_btn_hide(p->pool[i]);
    }
}

static GtkWidget *make_pool_button(EmojiPicker *p) {
    picker_css_ensure();
    GtkWidget *btn = gtk_button_new_with_label("");
    gtk_widget_set_size_request(btn, 40, 40);
    gtk_widget_set_can_focus(btn, FALSE);
    gtk_widget_set_focus_on_click(btn, FALSE);
    gtk_style_context_add_class(gtk_widget_get_style_context(btn), "emoji-cell");
    gtk_style_context_add_provider(gtk_widget_get_style_context(btn),
                                   GTK_STYLE_PROVIDER(g_btn_css),
                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_signal_connect(btn, "clicked", G_CALLBACK(picker_action_emoji_clicked), p);
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

void picker_grid_ensure_pool(EmojiPicker *p, int need) {
    if (need <= p->pool_cap)
        return;
    int nc = p->pool_cap ? p->pool_cap : 32;
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
    if (from < p->shown)
        p->shown = from;
}

static gboolean fill_idle_cb(gpointer user_data) {
    EmojiPicker *p = user_data;
    int end = p->fill_pos + FILL_CHUNK;
    if (end > p->reveal_to)
        end = p->reveal_to;
    picker_grid_ensure_pool(p, end);

    for (; p->fill_pos < end; p->fill_pos++) {
        int i = p->fill_pos;
        GtkWidget *btn = p->pool[i];
        const char *glyph = emoji_model_hit_glyph(p, i);
        const char *name = emoji_model_hit_name(p, i);
        gtk_button_set_label(GTK_BUTTON(btn), glyph);
        g_object_set_data(G_OBJECT(btn), "glyph", (gpointer)glyph);
        gtk_widget_set_tooltip_text(btn, name ? name : glyph);
        emoji_btn_show(btn);
    }
    if (p->fill_pos > p->shown)
        p->shown = p->fill_pos;

    if (p->fill_pos < p->reveal_to)
        return G_SOURCE_CONTINUE;

    p->fill_idle = 0;
    p->loading_more = FALSE;
    return G_SOURCE_REMOVE;
}

static void start_fill(EmojiPicker *p) {
    if (p->fill_idle) {
        g_source_remove(p->fill_idle);
        p->fill_idle = 0;
    }
    fill_idle_cb(p);
    if (p->fill_pos < p->reveal_to)
        p->fill_idle = g_idle_add(fill_idle_cb, p);
}

void picker_grid_present_reset(EmojiPicker *p) {
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

void picker_grid_on_vadj_changed(GtkAdjustment *adj, gpointer user_data) {
    EmojiPicker *p = user_data;
    double value = gtk_adjustment_get_value(adj);
    double page = gtk_adjustment_get_page_size(adj);
    double upper = gtk_adjustment_get_upper(adj);
    if (upper <= page)
        return;
    if (value + page >= upper - 64.0)
        load_more(p);
}
