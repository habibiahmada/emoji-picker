#ifndef POPUP_H
#define POPUP_H

#include <gtk/gtk.h>

typedef struct EmojiPicker EmojiPicker;

EmojiPicker *emoji_picker_new(void);
void emoji_picker_show(EmojiPicker *p);
GtkWidget *emoji_picker_window(EmojiPicker *p);

#endif
