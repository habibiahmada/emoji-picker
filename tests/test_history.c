#include "model/history.h"
#include "test_harness.h"

#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static char *g_tmpdir;

static void setup_tmpdir(void) {
    g_tmpdir = g_dir_make_tmp("emoji-picker-test-XXXXXX", NULL);
    TEST_ASSERT(g_tmpdir != NULL);
    char *data = g_build_filename(g_tmpdir, "share", NULL);
    g_setenv("XDG_DATA_HOME", data, TRUE);
    g_free(data);
}

static void cleanup_tmpdir(void) {
    if (!g_tmpdir)
        return;
    char *hist = g_build_filename(g_tmpdir, "share", "emoji-picker", "history",
                                  NULL);
    unlink(hist);
    g_free(hist);
    char *dir = g_build_filename(g_tmpdir, "share", "emoji-picker", NULL);
    rmdir(dir);
    g_free(dir);
    dir = g_build_filename(g_tmpdir, "share", NULL);
    rmdir(dir);
    g_free(dir);
    rmdir(g_tmpdir);
    g_free(g_tmpdir);
    g_tmpdir = NULL;
}

static void test_push_dedup_and_order(void) {
    emoji_history_load();
    TEST_ASSERT(emoji_history_count() == 0);

    emoji_history_push("😀");
    emoji_history_push("🐱");
    emoji_history_push("😀");
    TEST_ASSERT(emoji_history_count() == 2);
    TEST_ASSERT(strcmp(emoji_history_get(0), "😀") == 0);
    TEST_ASSERT(strcmp(emoji_history_get(1), "🐱") == 0);

    /* Reload from disk. */
    emoji_history_load();
    TEST_ASSERT(emoji_history_count() == 2);
    TEST_ASSERT(strcmp(emoji_history_get(0), "😀") == 0);
}

int main(void) {
    setup_tmpdir();
    TEST_RUN(test_push_dedup_and_order);
    cleanup_tmpdir();
    return test_finish();
}
