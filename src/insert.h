#ifndef INSERT_H
#define INSERT_H

#include <stdint.h>

/* Inserts by typing into the previously focused X11 window (no clipboard).
   Set via emoji_insert_set_target(); 0 = type into whatever is focused. */
void emoji_insert_set_target(uint32_t xid);

/* Picker window to restore after typing (keeps multi-pick open). 0 = skip. */
void emoji_insert_set_picker(uint32_t xid);

void emoji_insert(const char *glyph);

#endif
