#include "popup.h"

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCK_NAME "emoji-picker.sock"

static EmojiPicker *g_picker;
static int g_sock = -1;

static char *sock_path(void) {
    const char *runtime = getenv("XDG_RUNTIME_DIR");
    if (!runtime || !runtime[0])
        runtime = "/tmp";
    return g_strdup_printf("%s/%s", runtime, SOCK_NAME);
}

static gboolean on_sock_readable(GIOChannel *ch, GIOCondition cond, gpointer data) {
    (void)cond;
    (void)data;
    char buf[64];
    gsize n = 0;
    GError *err = NULL;
    if (g_io_channel_read_chars(ch, buf, sizeof(buf) - 1, &n, &err) != G_IO_STATUS_NORMAL) {
        if (err)
            g_error_free(err);
        return TRUE;
    }
    buf[n] = 0;
    if (g_str_has_prefix(buf, "show") || g_str_has_prefix(buf, "toggle")) {
        if (gtk_widget_get_visible(emoji_picker_window(g_picker)))
            gtk_widget_hide(emoji_picker_window(g_picker));
        else
            emoji_picker_show(g_picker);
    } else if (g_str_has_prefix(buf, "hide")) {
        gtk_widget_hide(emoji_picker_window(g_picker));
    }
    return TRUE;
}

static int setup_socket(void) {
    struct sockaddr_un addr;
    char *path = sock_path();
    unlink(path);
    int fd = socket(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
    if (fd < 0) {
        g_free(path);
        return -1;
    }
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        g_free(path);
        return -1;
    }
    g_free(path);
    GIOChannel *ch = g_io_channel_unix_new(fd);
    g_io_channel_set_encoding(ch, NULL, NULL);
    g_io_add_watch(ch, G_IO_IN, on_sock_readable, NULL);
    g_sock = fd;
    return 0;
}

static int send_cmd(const char *cmd) {
    struct sockaddr_un addr;
    char *path = sock_path();
    int fd = socket(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        g_free(path);
        return 1;
    }
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);
    int rc = sendto(fd, cmd, strlen(cmd), 0, (struct sockaddr *)&addr, sizeof(addr)) < 0;
    close(fd);
    g_free(path);
    return rc ? 1 : 0;
}

static gboolean on_delete(GtkWidget *w, GdkEvent *e, gpointer data) {
    (void)w;
    (void)e;
    (void)data;
    /* Hide instead of quit — daemon stays alive for instant reopen. */
    gtk_widget_hide(emoji_picker_window(g_picker));
    return TRUE;
}

int main(int argc, char **argv) {
    if (argc >= 2 && strcmp(argv[1], "--show") == 0)
        return send_cmd("show");
    if (argc >= 2 && strcmp(argv[1], "--toggle") == 0)
        return send_cmd("toggle");
    if (argc >= 2 && strcmp(argv[1], "--hide") == 0)
        return send_cmd("hide");

    gtk_init(&argc, &argv);

    g_picker = emoji_picker_new();
    g_signal_connect(emoji_picker_window(g_picker), "delete-event",
                     G_CALLBACK(on_delete), NULL);

    if (setup_socket() != 0) {
        g_printerr("emoji-picker: failed to bind socket (another instance?)\n");
        /* Still allow one-shot show */
        emoji_picker_show(g_picker);
        gtk_main();
        return 0;
    }

    gboolean show_now = TRUE;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--daemon") == 0)
            show_now = FALSE;
    }
    if (show_now)
        emoji_picker_show(g_picker);

    gtk_main();
    return 0;
}
