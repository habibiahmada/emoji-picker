#include "ui/popup.h"

#include "model/emoji_model.h"
#include "model/history.h"
#include "insert/insert.h"
#include "ui/picker_dismiss.h"
#include "ui/picker_grid.h"
#include "ui/picker_priv.h"
#include "model/search_query.h"
#include "platform/x11_util.h"

#include <string.h>

static const char *const tab_icon_names[TAB_COUNT] = {
    "emoji-recent-symbolic",
    "view-grid-symbolic",
    "face-smile-symbolic",
    "emoji-people-symbolic",
    "emoji-nature-symbolic",
    "emoji-food-symbolic",
    "emoji-travel-symbolic",
    "emoji-activities-symbolic",
    "emoji-objects-symbolic",
    "emoji-symbols-symbolic",
    "emoji-flags-symbolic",
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

static void remember_position(EmojiPicker *p) {
    int x = 0, y = 0;
    gtk_window_get_position(GTK_WINDOW(p->win), &x, &y);
    if (x > 0 || y > 0) {
        p->pos_x = x;
        p->pos_y = y;
        p->pos_valid = TRUE;
    }
}

static void rebuild_visible(EmojiPicker *p) {
    const char *q = gtk_entry_get_text(GTK_ENTRY(p->search));
    char query[256];
    search_ascii_lower_copy(q, query, sizeof(query));

    if (query[0])
        emoji_model_collect_search(p, query);
    else {
        int tab = gtk_notebook_get_current_page(GTK_NOTEBOOK(p->notebook));
        if (tab < 0 || tab >= TAB_COUNT)
            tab = TAB_ALL;
        emoji_model_collect_for_tab(p, tab);
    }
    picker_grid_present_reset(p);
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

static gboolean on_search_button_press(GtkWidget *w, GdkEventButton *ev,
                                       gpointer user_data) {
    (void)w;
    (void)ev;
    EmojiPicker *p = user_data;
    gtk_window_set_accept_focus(GTK_WINDOW(p->win), TRUE);
    return FALSE;
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
    emoji_model_collect_for_tab(p, (int)page_num);
    picker_grid_present_reset(p);
}

static gboolean on_key_press(GtkWidget *w, GdkEventKey *ev, gpointer user_data) {
    (void)w;
    EmojiPicker *p = user_data;
    if (ev->keyval == GDK_KEY_Escape) {
        picker_hide(p);
        return TRUE;
    }
    return FALSE;
}

static gboolean clear_status_cb(gpointer user_data) {
    EmojiPicker *p = user_data;
    p->status_timeout = 0;
    if (p->status)
        gtk_label_set_text(GTK_LABEL(p->status), "");
    return G_SOURCE_REMOVE;
}

static void set_status(EmojiPicker *p, const char *msg) {
    if (!p->status)
        return;
    gtk_label_set_text(GTK_LABEL(p->status), msg ? msg : "");
    if (p->status_timeout)
        g_source_remove(p->status_timeout);
    p->status_timeout = g_timeout_add(2500, clear_status_cb, p);
}

static void ungrab_seat(GtkWidget *w) {
    GdkDisplay *dpy = gtk_widget_get_display(w);
    if (!dpy)
        return;
    GdkSeat *seat = gdk_display_get_default_seat(dpy);
    if (seat)
        gdk_seat_ungrab(seat);
}

static gboolean finish_insert_cb(gpointer user_data) {
    EmojiPicker *p = user_data;
    p->finish_insert_timeout = 0;
    p->suppress_hide = FALSE;
    gtk_window_set_accept_focus(GTK_WINDOW(p->win), TRUE);
    if (!p->dismiss_poll)
        picker_dismiss_poll_start(p);
    return G_SOURCE_REMOVE;
}

typedef struct {
    EmojiPicker *p;
    char *glyph;
} InsertReq;

static gboolean deferred_insert_cb(gpointer user_data) {
    InsertReq *req = user_data;
    EmojiPicker *p = req->p;

    ungrab_seat(p->win);
    p->suppress_hide = TRUE;
    remember_position(p);
    gtk_window_set_accept_focus(GTK_WINDOW(p->win), FALSE);

    while (gtk_events_pending())
        gtk_main_iteration_do(FALSE);
    GdkDisplay *gd = gdk_display_get_default();
    if (gd)
        gdk_display_sync(gd);

    emoji_insert(req->glyph);

    if (p->finish_insert_timeout)
        g_source_remove(p->finish_insert_timeout);
    p->finish_insert_timeout = g_timeout_add(500, finish_insert_cb, p);

    char *msg = g_strdup_printf("Inserted %s", req->glyph);
    set_status(p, msg);
    g_free(msg);

    g_free(req->glyph);
    g_free(req);
    return G_SOURCE_REMOVE;
}

void picker_action_emoji_clicked(GtkButton *btn, gpointer user_data) {
    EmojiPicker *p = user_data;
    const char *glyph = g_object_get_data(G_OBJECT(btn), "glyph");
    if (!glyph)
        return;

    remember_position(p);
    emoji_history_push(glyph);

    InsertReq *req = g_new(InsertReq, 1);
    req->p = p;
    req->glyph = g_strdup(glyph);
    g_timeout_add(100, deferred_insert_cb, req);
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

static GtkWidget *make_tab_label(int i) {
    GtkWidget *img =
        gtk_image_new_from_icon_name(tab_icon_names[i], GTK_ICON_SIZE_MENU);
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
    picker_css_ensure();

    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(root), 10);
    gtk_container_add(GTK_CONTAINER(p->win), root);

    p->search = gtk_entry_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(p->search),
                                "emoji-search");
    gtk_style_context_add_provider(gtk_widget_get_style_context(p->search),
                                   GTK_STYLE_PROVIDER(picker_css_buttons()),
                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    gtk_entry_set_placeholder_text(GTK_ENTRY(p->search),
                                   "Search name or emoji…");
    g_signal_connect(p->search, "changed", G_CALLBACK(on_search_changed), p);
    g_signal_connect(p->search, "button-press-event",
                     G_CALLBACK(on_search_button_press), p);
    gtk_box_pack_start(GTK_BOX(root), p->search, FALSE, FALSE, 0);

    p->notebook = gtk_notebook_new();
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(p->notebook), TRUE);
    gtk_notebook_set_show_border(GTK_NOTEBOOK(p->notebook), FALSE);
    gtk_style_context_add_provider(gtk_widget_get_style_context(p->notebook),
                                   GTK_STYLE_PROVIDER(picker_css_tabs()),
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
    g_signal_connect(vadj, "value-changed", G_CALLBACK(picker_grid_on_vadj_changed),
                     p);

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
                                   GTK_STYLE_PROVIDER(picker_css_buttons()),
                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    gtk_container_add(GTK_CONTAINER(p->scroll), p->flow);

    p->status = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(p->status), 0.0f);
    gtk_widget_set_opacity(p->status, 0.75);
    gtk_box_pack_start(GTK_BOX(root), p->status, FALSE, FALSE, 0);

    g_signal_connect(p->notebook, "switch-page", G_CALLBACK(on_page_switched), p);
    g_signal_connect(p->win, "key-press-event", G_CALLBACK(on_key_press), p);

    emoji_model_build_indexes(p);
    picker_grid_ensure_pool(p, PAGE_SIZE);

    int start = emoji_history_count() > 0 ? TAB_HISTORY : TAB_ALL;
    gtk_notebook_set_current_page(GTK_NOTEBOOK(p->notebook), start);
    emoji_model_collect_for_tab(p, start);
    picker_grid_present_reset(p);

    return p;
}

void emoji_picker_show(EmojiPicker *p) {
    uint32_t target = x11_capture_active_xid();

    gtk_window_set_keep_above(GTK_WINDOW(p->win), TRUE);
    gtk_window_set_accept_focus(GTK_WINDOW(p->win), TRUE);
    p->suppress_hide = FALSE;
    p->last_insert_us = 0;
    p->keymap_ready = FALSE;
    p->outside_btn_latched = FALSE;
    if (p->finish_insert_timeout) {
        g_source_remove(p->finish_insert_timeout);
        p->finish_insert_timeout = 0;
    }
    if (p->status)
        gtk_label_set_text(GTK_LABEL(p->status), "");

    gtk_widget_show_all(p->win);
    picker_grid_resync_visibility(p);

    place_near_pointer(p->win);
    remember_position(p);
    gtk_window_present(GTK_WINDOW(p->win));
    gtk_widget_grab_focus(p->search);

    uint32_t picker = x11_widget_xid(p->win);
    if (target != 0 && target != picker) {
        emoji_insert_set_target(target);
        p->sticky_target = target;
    } else {
        p->sticky_target = 0;
    }

    picker_dismiss_poll_start(p);

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
    emoji_model_collect_for_tab(p, start);
    picker_grid_present_reset(p);
}

GtkWidget *emoji_picker_window(EmojiPicker *p) {
    return p->win;
}
