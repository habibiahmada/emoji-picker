#ifndef PICKER_GRID_H
#define PICKER_GRID_H

#include "ui/picker_priv.h"

/* popup.c implements this; grid connects buttons to it. */
void picker_action_emoji_clicked(GtkButton *btn, gpointer user_data);

void picker_grid_ensure_pool(EmojiPicker *p, int need);
void picker_grid_present_reset(EmojiPicker *p);
void picker_grid_resync_visibility(EmojiPicker *p);
void picker_grid_on_vadj_changed(GtkAdjustment *adj, gpointer user_data);

GtkCssProvider *picker_css_buttons(void);
GtkCssProvider *picker_css_tabs(void);
void picker_css_ensure(void);

#endif
