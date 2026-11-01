#ifndef PICKER_TONE_H
#define PICKER_TONE_H

#include "ui/picker_priv.h"

/* Skin-tone chooser for base entry `base_idx`, anchored on a grid cell. */
void picker_tone_popover_show(EmojiPicker *p, GtkWidget *anchor, int base_idx);
void picker_tone_popover_close(EmojiPicker *p);

#endif
