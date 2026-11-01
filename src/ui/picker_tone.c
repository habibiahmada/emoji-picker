#include "ui/picker_tone.h"

#include "model/emoji_model.h"
#include "model/skin_tone.h"
#include "ui/picker_grid.h"

typedef struct {
    EmojiPicker *p;
    int base;
    int left;                                /* first person's tone, 1..5 */
    GtkWidget *swatches[SKIN_TONE_MAX];
    GtkWidget *pairs[SKIN_TONE_MAX];
} ToneMenu;

static const char *const tone_tips[SKIN_TONE_MAX + 1] = {
    "Default", "Light", "Medium-light", "Medium", "Medium-dark", "Dark",
};

static void on_option_clicked(GtkButton *btn, gpointer user_data) {
    ToneMenu *m = user_data;
    EmojiPicker *p = m->p;
    char *glyph = g_strdup(g_object_get_data(G_OBJECT(btn), "glyph"));
    int tone = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(btn), "tone"));

    /* Uniform picks become the saved tone for every toned emoji. */
    if (tone >= SKIN_TONE_DEFAULT) {
        skin_tone_set(tone);
        picker_grid_refresh_labels(p);
    }
    gtk_popover_popdown(GTK_POPOVER(p->tone_popover));
    picker_insert_glyph(p, glyph);
    g_free(glyph);
}

static GtkWidget *make_option(ToneMenu *m, const char *glyph, int tone,
                              const char *tip) {
    GtkWidget *btn = gtk_button_new_with_label(glyph);
    gtk_widget_set_can_focus(btn, FALSE);
    gtk_widget_set_focus_on_click(btn, FALSE);
    GtkStyleContext *sc = gtk_widget_get_style_context(btn);
    gtk_style_context_add_class(sc, "emoji-cell");
    gtk_style_context_add_provider(sc, GTK_STYLE_PROVIDER(picker_css_buttons()),
                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    if (tone >= SKIN_TONE_DEFAULT && tone == skin_tone_get())
        gtk_style_context_add_class(sc, "emoji-tone-current");
    gtk_widget_set_tooltip_text(btn, tip);
    g_object_set_data_full(G_OBJECT(btn), "glyph", g_strdup(glyph), g_free);
    g_object_set_data(G_OBJECT(btn), "tone", GINT_TO_POINTER(tone));
    g_signal_connect(btn, "clicked", G_CALLBACK(on_option_clicked), m);
    return btn;
}

static void update_pairs(ToneMenu *m) {
    for (int t = 1; t <= SKIN_TONE_MAX; t++) {
        GtkStyleContext *sc = gtk_widget_get_style_context(m->swatches[t - 1]);
        if (t == m->left)
            gtk_style_context_add_class(sc, "active");
        else
            gtk_style_context_remove_class(sc, "active");

        GtkWidget *btn = m->pairs[t - 1];
        const EmojiToneVariant *v = skin_tone_pair(m->base, m->left, t);
        if (!v || !emoji_model_version_supported(v->version)) {
            gtk_widget_hide(btn);
            continue;
        }
        gtk_button_set_label(GTK_BUTTON(btn), v->glyph);
        g_object_set_data_full(G_OBJECT(btn), "glyph", g_strdup(v->glyph), g_free);
        char *tip = g_strdup_printf("%s + %s", tone_tips[m->left], tone_tips[t]);
        gtk_widget_set_tooltip_text(btn, tip);
        g_free(tip);
        gtk_widget_show(btn);
    }
}

static void on_swatch_clicked(GtkButton *btn, gpointer user_data) {
    ToneMenu *m = user_data;
    m->left = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(btn), "tone"));
    update_pairs(m);
}

static gboolean destroy_idle(gpointer data) {
    gtk_widget_destroy(GTK_WIDGET(data));
    return G_SOURCE_REMOVE;
}

static void on_closed(GtkPopover *pop, gpointer user_data) {
    EmojiPicker *p = user_data;
    if (p->tone_popover != GTK_WIDGET(pop))
        return;
    p->tone_popover = NULL;
    /* Not from inside the click that closed it. */
    g_idle_add(destroy_idle, pop);
}

void picker_tone_popover_close(EmojiPicker *p) {
    GtkWidget *pop = p->tone_popover;
    if (!pop)
        return;
    p->tone_popover = NULL;
    gtk_widget_destroy(pop);
}

void picker_tone_popover_show(EmojiPicker *p, GtkWidget *anchor, int base_idx) {
    picker_tone_popover_close(p);

    ToneMenu *m = g_new0(ToneMenu, 1);
    m->p = p;
    m->base = base_idx;
    m->left = skin_tone_get() > SKIN_TONE_DEFAULT ? skin_tone_get() : 1;

    GtkWidget *pop = gtk_popover_new(anchor);
    g_object_set_data_full(G_OBJECT(pop), "tone-menu", m, g_free);
    gtk_popover_set_position(GTK_POPOVER(pop), GTK_POS_TOP);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_container_set_border_width(GTK_CONTAINER(box), 4);
    gtk_container_add(GTK_CONTAINER(pop), box);

    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
    gtk_box_pack_start(GTK_BOX(box), row, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row),
                       make_option(m, emoji_entries[base_idx].glyph,
                                   SKIN_TONE_DEFAULT, tone_tips[0]),
                       FALSE, FALSE, 0);
    for (int t = 1; t <= SKIN_TONE_MAX; t++) {
        const EmojiToneVariant *v = skin_tone_uniform(base_idx, t);
        if (!v || !emoji_model_version_supported(v->version))
            continue;
        gtk_box_pack_start(GTK_BOX(row), make_option(m, v->glyph, t, tone_tips[t]),
                           FALSE, FALSE, 0);
    }

    if (skin_tone_has_pairs(base_idx)) {
        gtk_box_pack_start(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL),
                           FALSE, FALSE, 2);
        GtkWidget *sw_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        gtk_widget_set_halign(sw_row, GTK_ALIGN_CENTER);
        gtk_box_pack_start(GTK_BOX(box), sw_row, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(sw_row), gtk_label_new("First person"), FALSE,
                           FALSE, 0);
        for (int t = 1; t <= SKIN_TONE_MAX; t++) {
            GtkWidget *sw = gtk_button_new();
            gtk_widget_set_can_focus(sw, FALSE);
            gtk_widget_set_valign(sw, GTK_ALIGN_CENTER);
            GtkStyleContext *sc = gtk_widget_get_style_context(sw);
            gtk_style_context_add_class(sc, "tone-swatch");
            char cls[8];
            g_snprintf(cls, sizeof(cls), "tone-%d", t);
            gtk_style_context_add_class(sc, cls);
            gtk_style_context_add_provider(sc, GTK_STYLE_PROVIDER(picker_css_buttons()),
                                           GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
            gtk_widget_set_tooltip_text(sw, tone_tips[t]);
            g_object_set_data(G_OBJECT(sw), "tone", GINT_TO_POINTER(t));
            g_signal_connect(sw, "clicked", G_CALLBACK(on_swatch_clicked), m);
            gtk_box_pack_start(GTK_BOX(sw_row), sw, FALSE, FALSE, 0);
            m->swatches[t - 1] = sw;
        }

        GtkWidget *pair_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
        gtk_widget_set_halign(pair_row, GTK_ALIGN_CENTER);
        gtk_box_pack_start(GTK_BOX(box), pair_row, FALSE, FALSE, 0);
        for (int t = 1; t <= SKIN_TONE_MAX; t++) {
            /* tone -1: mixed pick inserts but does not change the saved tone. */
            m->pairs[t - 1] = make_option(m, "", -1, NULL);
            gtk_widget_set_no_show_all(m->pairs[t - 1], TRUE);
            gtk_box_pack_start(GTK_BOX(pair_row), m->pairs[t - 1], FALSE, FALSE, 0);
        }
    }

    gtk_widget_show_all(box);
    if (m->pairs[0])
        update_pairs(m);

    p->tone_popover = pop;
    g_signal_connect(pop, "closed", G_CALLBACK(on_closed), p);
    gtk_popover_popup(GTK_POPOVER(pop));
}
