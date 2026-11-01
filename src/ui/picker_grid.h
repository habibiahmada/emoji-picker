#ifndef PICKER_GRID_H
#define PICKER_GRID_H

#include "ui/picker_priv.h"

/* popup.c implements these; grid and tone popover call them. */
void picker_action_emoji_clicked(GtkButton *btn, gpointer user_data);
void picker_insert_glyph(EmojiPicker *p, const char *glyph);

/* Scrollable grid widget to put in p->scroll; owns the button pool. */
GtkWidget *picker_grid_new(EmojiPicker *p);
void picker_grid_present_reset(EmojiPicker *p);
void picker_grid_resync_visibility(EmojiPicker *p);
/* Re-read glyphs of shown cells (after the saved skin tone changes). */
void picker_grid_refresh_labels(EmojiPicker *p);
void picker_grid_on_vadj_changed(GtkAdjustment *adj, gpointer user_data);

GtkCssProvider *picker_css_buttons(void);
GtkCssProvider *picker_css_tabs(void);
void picker_css_ensure(void);

#endif
