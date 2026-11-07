#include "model/search_query.h"

#include <ctype.h>
#include <string.h>

int search_tokenize_query(const char *q, char tokens[][EMOJI_SEARCH_TOKEN_LEN],
                          int max_tok) {
    if (!q || max_tok <= 0)
        return 0;

    int ntok = 0;
    size_t n = 0;
    for (const char *s = q;; s++) {
        unsigned char c = (unsigned char)*s;
        if (c && !isspace(c) && c != '-' && c != '_') {
            if (ntok < max_tok && n + 1 < EMOJI_SEARCH_TOKEN_LEN)
                tokens[ntok][n++] = (char)tolower(c);
            continue;
        }
        if (n > 0 && ntok < max_tok) {
            tokens[ntok][n] = 0;
            ntok++;
            n = 0;
        }
        if (!c)
            break;
    }
    return ntok;
}

int search_token_is_ascii_word(const char *tok) {
    if (!tok || !tok[0])
        return 0;
    for (const unsigned char *p = (const unsigned char *)tok; *p; p++) {
        if (*p >= 0x80)
            return 0;
        if (!isalnum(*p))
            return 0;
    }
    return 1;
}

/* UTF-8 bytes count as word characters so "piñata" stays one word. */
static int is_word_char(unsigned char c) {
    return c >= 0x80 || isalnum(c);
}

/* 2 = tok is a whole word of text, 1 = tok starts a word, 0 = neither. */
static int word_match(const char *text, const char *tok) {
    if (!text)
        return 0;
    size_t tlen = strlen(tok);
    int best = 0;
    for (const char *s = text; *s;) {
        while (*s && !is_word_char((unsigned char)*s))
            s++;
        const char *start = s;
        while (*s && is_word_char((unsigned char)*s))
            s++;
        if ((size_t)(s - start) >= tlen && strncmp(start, tok, tlen) == 0) {
            if ((size_t)(s - start) == tlen)
                return 2;
            best = 1;
        }
    }
    return best;
}

int search_token_score(const char *name, const char *keywords, const char *tok) {
    if (!name || !tok || !tok[0])
        return 0;
    if (strcmp(name, tok) == 0)
        return 5;
    int in_name = word_match(name, tok);
    if (in_name == 2)
        return 4;
    int in_keywords = word_match(keywords, tok);
    if (in_keywords == 2)
        return 3;
    if (in_name)
        return 2;
    return (in_keywords || strstr(name, tok)) ? 1 : 0;
}

size_t search_ascii_lower_copy(const char *q, char *out, size_t out_cap) {
    if (!out || out_cap == 0)
        return 0;
    size_t n = 0;
    if (!q) {
        out[0] = 0;
        return 0;
    }
    for (const char *s = q; *s && n + 1 < out_cap; s++) {
        unsigned char c = (unsigned char)*s;
        if (c >= 'A' && c <= 'Z')
            out[n++] = (char)(c - 'A' + 'a');
        else
            out[n++] = (char)c;
    }
    out[n] = 0;
    return n;
}
