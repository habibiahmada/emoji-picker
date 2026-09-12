#ifndef EMOJI_MODEL_H
#define EMOJI_MODEL_H

#include "ui/picker_priv.h"

void emoji_model_hits_reset(EmojiPicker *p);
void emoji_model_hits_push(EmojiPicker *p, uint16_t idx, const char *orphan_glyph);
const char *emoji_model_hit_glyph(EmojiPicker *p, int i);
const char *emoji_model_hit_name(EmojiPicker *p, int i);

void emoji_model_build_indexes(EmojiPicker *p);
void emoji_model_collect_for_tab(EmojiPicker *p, int tab);
void emoji_model_collect_search(EmojiPicker *p, const char *query);

#endif
