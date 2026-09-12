#ifndef SEARCH_QUERY_H
#define SEARCH_QUERY_H

#include <stddef.h>

#include <glib.h>

#define EMOJI_SEARCH_TOKEN_MAX 8
#define EMOJI_SEARCH_TOKEN_LEN 64

/* Split query on whitespace / - / _; ASCII letters lowercased. */
int search_tokenize_query(const char *q, char tokens[][EMOJI_SEARCH_TOKEN_LEN],
                          int max_tok);

/* True if token is non-empty ASCII alphanumeric (not an emoji glyph). */
int search_token_is_ascii_word(const char *tok);

/* Every token must appear as a substring of name (already lowercase). */
gboolean search_name_has_all_tokens(const char *name,
                                    char tokens[][EMOJI_SEARCH_TOKEN_LEN],
                                    int ntok);

/*
 * Copy q into out, lowercasing A–Z only (UTF-8 emoji glyphs stay intact).
 * Returns length written (excluding NUL).
 */
size_t search_ascii_lower_copy(const char *q, char *out, size_t out_cap);

#endif
