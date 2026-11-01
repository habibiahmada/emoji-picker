#ifndef FONT_SUPPORT_H
#define FONT_SUPPORT_H

#include <stdint.h>

/*
 * Newest emoji version (x10) the installed color emoji font fully covers,
 * judged by single-codepoint emoji. UINT16_MAX when it cannot tell, so a
 * missing/odd font never hides the whole grid.
 */
uint16_t font_emoji_max_version(void);

#endif
