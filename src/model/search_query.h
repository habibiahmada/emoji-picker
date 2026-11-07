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

/*
 * How well one lowercase token matches an emoji, best first:
 * 5 = the whole name, 4 = a whole name word, 3 = a whole keyword,
 * 2 = start of a name word, 1 = start of a keyword or anywhere in the name,
 * 0 = no match. A whole word beats a prefix: "rock" puts 🗿 before 🚀.
 * keywords may be NULL.
 */
int search_token_score(const char *name, const char *keywords, const char *tok);

/*
 * Copy q into out, lowercasing A–Z only (UTF-8 emoji glyphs stay intact).
 * Returns length written (excluding NUL).
 */
size_t search_ascii_lower_copy(const char *q, char *out, size_t out_cap);

#endif
