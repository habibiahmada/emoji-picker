#include "model/history.h"

#include <glib.h>
#include <stdio.h>
#include <string.h>

static char *g_hist[EMOJI_HISTORY_MAX];
static int g_hist_n;
static gboolean g_dirty;

static char *history_path(void) {
    const char *data = g_get_user_data_dir();
    return g_build_filename(data, "emoji-picker", "history", NULL);
}

void emoji_history_load(void) {
    for (int i = 0; i < g_hist_n; i++) {
        g_free(g_hist[i]);
        g_hist[i] = NULL;
    }
    g_hist_n = 0;
    g_dirty = FALSE;

    char *path = history_path();
    FILE *fp = fopen(path, "r");
    g_free(path);
    if (!fp)
        return;

    char line[64];
    while (g_hist_n < EMOJI_HISTORY_MAX && fgets(line, sizeof(line), fp)) {
        size_t n = strlen(line);
        while (n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r'))
            line[--n] = 0;
        if (n == 0)
            continue;
        g_hist[g_hist_n++] = g_strdup(line);
    }
    fclose(fp);
}

void emoji_history_save(void) {
    if (!g_dirty)
        return;
    char *dir = g_build_filename(g_get_user_data_dir(), "emoji-picker", NULL);
    g_mkdir_with_parents(dir, 0700);
    char *path = g_build_filename(dir, "history", NULL);
    g_free(dir);

    FILE *fp = fopen(path, "w");
    g_free(path);
    if (!fp)
        return;
    for (int i = 0; i < g_hist_n; i++)
        fprintf(fp, "%s\n", g_hist[i]);
    fclose(fp);
    g_dirty = FALSE;
}

void emoji_history_push(const char *glyph) {
    if (!glyph || !glyph[0])
        return;

    /* Remove existing duplicate. */
    for (int i = 0; i < g_hist_n; i++) {
        if (strcmp(g_hist[i], glyph) == 0) {
            char *dup = g_hist[i];
            memmove(&g_hist[1], &g_hist[0], (size_t)i * sizeof(g_hist[0]));
            g_hist[0] = dup;
            g_dirty = TRUE;
            emoji_history_save();
            return;
        }
    }

    if (g_hist_n == EMOJI_HISTORY_MAX) {
        g_free(g_hist[EMOJI_HISTORY_MAX - 1]);
        g_hist_n--;
    }
    memmove(&g_hist[1], &g_hist[0], (size_t)g_hist_n * sizeof(g_hist[0]));
    g_hist[0] = g_strdup(glyph);
    g_hist_n++;
    g_dirty = TRUE;
    emoji_history_save();
}

int emoji_history_count(void) {
    return g_hist_n;
}

const char *emoji_history_get(int i) {
    if (i < 0 || i >= g_hist_n)
        return NULL;
    return g_hist[i];
}
