#ifndef EMOJI_MODEL_H
#define EMOJI_MODEL_H

#include "ui/picker_priv.h"

/* hits[] value for a History glyph that is not in the Unicode table. */
#define EMOJI_NO_INDEX UINT16_MAX

void emoji_model_hits_reset(EmojiPicker *p);
void emoji_model_hits_push(EmojiPicker *p, uint16_t idx, const char *orphan_glyph);
const char *emoji_model_hit_glyph(EmojiPicker *p, int i);
const char *emoji_model_hit_name(EmojiPicker *p, int i);
/* Base entry index when hit i follows the saved skin tone, else -1. */
int emoji_model_hit_tone_base(EmojiPicker *p, int i);

/* Hide entries newer than the installed emoji font can draw. */
void emoji_model_set_max_version(uint16_t version);
int emoji_model_version_supported(uint16_t version);

void emoji_model_build_indexes(EmojiPicker *p);
void emoji_model_collect_for_tab(EmojiPicker *p, int tab);
void emoji_model_collect_search(EmojiPicker *p, const char *query);

#endif
