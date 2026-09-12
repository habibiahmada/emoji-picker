#ifndef INSERT_H
#define INSERT_H

#include <stdint.h>

void emoji_insert_set_target(uint32_t xid);

int emoji_copy(const char *glyph);

/* Clear CLIPBOARD (empty xclip). Does not wipe clipboard-manager history apps. */
void emoji_clipboard_clear(void);

/*
 * xclip + Ctrl+V into pre-picker window. Caller must ungrab first.
 * Clears CLIPBOARD ~450ms after paste. Picker can stay visible.
 */
void emoji_insert(const char *glyph);

#endif
