#include "model/search_query.h"
#include "test_harness.h"

#include <string.h>

static void test_tokenize_basic(void) {
    char tokens[8][64];
    int n = search_tokenize_query("grinning face", tokens, 8);
    TEST_ASSERT(n == 2);
    TEST_ASSERT(strcmp(tokens[0], "grinning") == 0);
    TEST_ASSERT(strcmp(tokens[1], "face") == 0);
}

static void test_tokenize_hyphen_lower(void) {
    char tokens[8][64];
    int n = search_tokenize_query("OK-Hand", tokens, 8);
    TEST_ASSERT(n == 2);
    TEST_ASSERT(strcmp(tokens[0], "ok") == 0);
    TEST_ASSERT(strcmp(tokens[1], "hand") == 0);
}

static void test_ascii_word(void) {
    TEST_ASSERT(search_token_is_ascii_word("cat") == 1);
    TEST_ASSERT(search_token_is_ascii_word("a1") == 1);
    TEST_ASSERT(search_token_is_ascii_word("🐸") == 0);
    TEST_ASSERT(search_token_is_ascii_word("") == 0);
}

static void test_token_score_order(void) {
    const char *kw = "face moyai statue stoneface";
    TEST_ASSERT(search_token_score("moai", kw, "moai") == 5);
    TEST_ASSERT(search_token_score("grinning face", NULL, "face") == 4);
    TEST_ASSERT(search_token_score("moai", kw, "statue") == 3);
    TEST_ASSERT(search_token_score("grinning face", NULL, "grin") == 2);
    TEST_ASSERT(search_token_score("moai", kw, "stone") == 1);
    TEST_ASSERT(search_token_score("grinning face", NULL, "inning") == 1);
    TEST_ASSERT(search_token_score("moai", kw, "rock") == 0);
}

static void test_token_score_word_bounds(void) {
    /* "flag:" and "piñata" are single words; the colon is not part of it. */
    TEST_ASSERT(search_token_score("flag: indonesia", NULL, "flag") == 4);
    TEST_ASSERT(search_token_score("piñata", NULL, "pi") == 2);
    TEST_ASSERT(search_token_score("piñata", NULL, "ata") == 1);
    TEST_ASSERT(search_token_score("moai", "", "") == 0);
}

static void test_ascii_lower_preserves_emoji(void) {
    char out[64];
    search_ascii_lower_copy("ABC🐸Xy", out, sizeof(out));
    TEST_ASSERT(strcmp(out, "abc🐸xy") == 0);
}

int main(void) {
    TEST_RUN(test_tokenize_basic);
    TEST_RUN(test_tokenize_hyphen_lower);
    TEST_RUN(test_ascii_word);
    TEST_RUN(test_token_score_order);
    TEST_RUN(test_token_score_word_bounds);
    TEST_RUN(test_ascii_lower_preserves_emoji);
    return test_finish();
}
