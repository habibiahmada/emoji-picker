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

static void test_name_tokens(void) {
    char tokens[2][64] = {"grin", "face"};
    TEST_ASSERT(search_name_has_all_tokens("grinning face", tokens, 2));
    TEST_ASSERT(!search_name_has_all_tokens("smiling face", tokens, 2));
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
    TEST_RUN(test_name_tokens);
    TEST_RUN(test_ascii_lower_preserves_emoji);
    return test_finish();
}
