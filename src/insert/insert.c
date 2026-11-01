#include "insert/insert.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <glib.h>
#include <gtk/gtk.h>

/* Clear only after Ctrl+V was actually sent, so Brave has read CLIPBOARD. */
#define CLEAR_AFTER_PASTE_MS 450

static uint32_t g_target_xid;
static GQueue g_pending = G_QUEUE_INIT;
static gboolean g_paste_busy;
static guint g_clear_timeout;

void emoji_insert_set_target(uint32_t xid) {
    g_target_xid = xid;
}

static void reap_child(GPid pid, gint status, gpointer user_data) {
    (void)status;
    (void)user_data;
    g_spawn_close_pid(pid);
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

static gboolean have_xclip(void) {
    static int cached = -1;
    if (cached < 0) {
        char *path = g_find_program_in_path("xclip");
        cached = path != NULL;
        g_free(path);
    }
    return cached;
}

static gboolean clear_after_paste_cb(gpointer data) {
    (void)data;
    g_clear_timeout = 0;
    emoji_clipboard_clear();
    return G_SOURCE_REMOVE;
}

static void paste_next(void);

static void paste_done(GPid pid, gint status, gpointer user_data) {
    (void)status;
    (void)user_data;
    g_spawn_close_pid(pid);
    g_paste_busy = FALSE;
    if (!g_queue_is_empty(&g_pending)) {
        paste_next();
        return;
    }
    g_clear_timeout = g_timeout_add(CLEAR_AFTER_PASTE_MS, clear_after_paste_cb, NULL);
}

/*
 * Chromium needs clipboard Ctrl+V. Caller must ungrab first.
 *
 * Copy, focus and Ctrl+V run in order inside one shell: xclip -i only returns
 * after it owns CLIPBOARD, so Ctrl+V never races a still-starting xclip. Runs
 * async — the GTK loop is never blocked waiting on xclip/xdotool.
 *
 * Use windowfocus (no raise) — not windowactivate. Activate + toggling
 * keep_above made the picker appear to blink (Brave jumps in front, then
 * picker jumps back). Picker stays keep_above; focus moves for Ctrl+V only.
 */
static void paste_next(void) {
    char *glyph = g_queue_pop_head(&g_pending);
    if (!glyph)
        return;

    gboolean xclip = have_xclip();
    if (!xclip && clipboard_via_gtk(glyph) != 0) {
        g_free(glyph);
        paste_next();
        return;
    }

    char id[32];
    g_snprintf(id, sizeof(id), "%u", (unsigned)g_target_xid);
    char *argv[] = {
        "sh", "-c",
        "if [ -n \"$3\" ]; then "
        "printf %s \"$2\" | xclip -selection clipboard -t UTF8_STRING -i || exit 1; "
        "fi; "
        "[ \"$1\" = 0 ] && exit 0; "
        "sleep 0.05; "
        "xdotool windowfocus \"$1\"; "
        "sleep 0.10; "
        "xdotool key --clearmodifiers ctrl+v",
        "sh", id, glyph, xclip ? "1" : "", NULL};

    GPid pid = 0;
    GError *err = NULL;
    if (!g_spawn_async(NULL, argv, NULL,
                       G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD |
                           G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL,
                       NULL, NULL, &pid, &err)) {
        g_clear_error(&err);
        g_free(glyph);
        paste_next();
        return;
    }
    g_free(glyph);
    g_paste_busy = TRUE;
    g_child_watch_add(pid, paste_done, NULL);
}

void emoji_insert(const char *glyph) {
    if (!glyph || !glyph[0])
        return;
    /* A newer pick owns CLIPBOARD now; do not wipe it before its paste. */
    if (g_clear_timeout) {
        g_source_remove(g_clear_timeout);
        g_clear_timeout = 0;
    }
    g_queue_push_tail(&g_pending, g_strdup(glyph));
    if (!g_paste_busy)
        paste_next();
}
