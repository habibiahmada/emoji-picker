#ifndef HISTORY_H
#define HISTORY_H

#define EMOJI_HISTORY_MAX 64

void emoji_history_load(void);
void emoji_history_save(void);
void emoji_history_push(const char *glyph);
int emoji_history_count(void);
const char *emoji_history_get(int i); /* 0 = most recent */

#endif
