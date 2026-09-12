#include "insert/insert.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <glib.h>
#include <gtk/gtk.h>

static uint32_t g_target_xid;
static guint g_insert_serial;

void emoji_insert_set_target(uint32_t xid) {
    g_target_xid = xid;
}

static void reap_child(GPid pid, gint status, gpointer user_data) {
    (void)status;
    (void)user_data;
    g_spawn_close_pid(pid);
}

static int spawn_reaped(char **argv) {
    GPid pid = 0;
    GError *err = NULL;
    if (!g_spawn_async(NULL, argv, NULL,
                       G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD |
                           G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL,
                       NULL, NULL, &pid, &err)) {
        g_clear_error(&err);
        return -1;
    }
    g_child_watch_add(pid, reap_child, NULL);
    return 0;
}

static int clipboard_via_xclip(const char *glyph) {
    char *argv[] = {"xclip", "-selection", "clipboard", "-t", "UTF8_STRING", "-i",
                    NULL};
    GPid pid = 0;
    gint stdin_fd = -1;
    GError *err = NULL;

    if (!g_spawn_async_with_pipes(
            NULL, argv, NULL,
            G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD |
                G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL,
            NULL, NULL, &pid, &stdin_fd, NULL, NULL, &err)) {
        g_clear_error(&err);
        return -1;
    }

    g_child_watch_add(pid, reap_child, NULL);

    if (glyph && glyph[0]) {
        const char *p = glyph;
        gsize left = strlen(glyph);
        while (left > 0) {
            gssize n = write(stdin_fd, p, left);
            if (n < 0) {
                if (errno == EINTR)
                    continue;
                close(stdin_fd);
                return -1;
            }
            p += (gsize)n;
            left -= (gsize)n;
        }
    }
    close(stdin_fd);
    return 0;
}

static void clipboard_get(GtkClipboard *clipboard, GtkSelectionData *selection_data,
                          guint info, gpointer user_data) {
    const char *text = user_data;
    (void)clipboard;
    (void)info;
    if (text)
        gtk_selection_data_set_text(selection_data, text, -1);
}

static void clipboard_clear_cb(GtkClipboard *clipboard, gpointer user_data) {
    (void)clipboard;
    g_free(user_data);
}

static int clipboard_via_gtk(const char *glyph) {
    static const GtkTargetEntry targets[] = {
        {"UTF8_STRING", 0, 0},
        {"TEXT", 0, 0},
        {"STRING", 0, 0},
        {"text/plain", 0, 0},
        {"text/plain;charset=utf-8", 0, 0},
    };
    char *dup = g_strdup(glyph ? glyph : "");
    if (!dup)
        return -1;
    GtkClipboard *cb = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    if (!gtk_clipboard_set_with_data(cb, targets, G_N_ELEMENTS(targets),
                                     clipboard_get, clipboard_clear_cb, dup)) {
        g_free(dup);
        return -1;
    }
    return 0;
}

int emoji_copy(const char *glyph) {
    if (!glyph || !glyph[0])
        return -1;
    if (clipboard_via_xclip(glyph) == 0)
        return 0;
    return clipboard_via_gtk(glyph);
}

void emoji_clipboard_clear(void) {
    if (clipboard_via_xclip("") == 0)
        return;
    GtkClipboard *cb = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_clear(cb);
}

static gboolean clear_after_paste_cb(gpointer data) {
    guint serial = GPOINTER_TO_UINT(data);
    /* Skip if a newer insert already replaced the clipboard. */
    if (serial == g_insert_serial)
        emoji_clipboard_clear();
    return G_SOURCE_REMOVE;
}

/*
 * Chromium needs clipboard Ctrl+V. Caller must ungrab first.
 *
 * Use windowfocus (no raise) — not windowactivate. Activate + toggling
 * keep_above made the picker appear to blink (Brave jumps in front, then
 * picker jumps back). Picker stays keep_above; focus moves for Ctrl+V only.
 */
static void paste_to_target(void) {
    if (!g_target_xid)
        return;

    char id[32];
    g_snprintf(id, sizeof(id), "%u", (unsigned)g_target_xid);

    char *argv[] = {
        "sh", "-c",
        "sleep 0.05; "
        "xdotool windowfocus \"$1\"; "
        "sleep 0.10; "
        "xdotool key --clearmodifiers ctrl+v",
        "sh", id, NULL};

    spawn_reaped(argv);
}

void emoji_insert(const char *glyph) {
    if (!glyph || !glyph[0])
        return;
    g_insert_serial++;
    if (emoji_copy(glyph) != 0)
        return;
    paste_to_target();
    /* After Brave reads CLIPBOARD, drop contents so it is not left in history. */
    g_timeout_add(450, clear_after_paste_cb, GUINT_TO_POINTER(g_insert_serial));
}
