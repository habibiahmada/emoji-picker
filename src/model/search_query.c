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

gboolean search_name_has_all_tokens(const char *name,
                                    char tokens[][EMOJI_SEARCH_TOKEN_LEN],
                                    int ntok) {
    if (!name)
        return FALSE;
    for (int t = 0; t < ntok; t++) {
        if (!strstr(name, tokens[t]))
            return FALSE;
    }
    return TRUE;
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
