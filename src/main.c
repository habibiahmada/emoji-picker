#include "ui/popup.h"

#include <glib-unix.h>
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

static void handle_cmd(const char *buf) {
    GtkWidget *win = emoji_picker_window(g_picker);
    gboolean visible = gtk_widget_get_visible(win);
    gboolean want;
    if (g_str_has_prefix(buf, "toggle"))
        want = !visible;
    else if (g_str_has_prefix(buf, "show"))
        want = TRUE;
    else if (g_str_has_prefix(buf, "hide"))
        want = FALSE;
    else
        return;
    if (want && !visible)
        emoji_picker_show(g_picker);
    else if (!want && visible)
        gtk_widget_hide(win);
}

/* recv() per datagram: a buffered GIOChannel merged back-to-back toggles. */
static gboolean on_sock_readable(gint fd, GIOCondition cond, gpointer data) {
    (void)cond;
    (void)data;
    char buf[64];
    for (;;) {
        ssize_t n = recv(fd, buf, sizeof(buf) - 1, 0);
        if (n < 0)
            break;
        buf[n] = 0;
        handle_cmd(buf);
    }
    return G_SOURCE_CONTINUE;
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
    g_unix_fd_add(fd, G_IO_IN, on_sock_readable, NULL);
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

#ifndef APP_VERSION
#define APP_VERSION "0.4.0"
#endif

int main(int argc, char **argv) {
    if (argc >= 2 && (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0)) {
        g_print("emoji-picker %s\n", APP_VERSION);
        return 0;
    }
    if (argc >= 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        g_print("Usage: emoji-picker [OPTIONS]\n\n"
                "Lightweight Unicode emoji picker for Linux X11 desktops.\n\n"
                "Options:\n"
                "  --toggle       Show or hide the picker window (shortcut target)\n"
                "  --show         Explicitly show the picker window\n"
                "  --hide         Explicitly hide the picker window\n"
                "  --daemon       Run resident in background without opening window\n"
                "  -v, --version  Print version information\n"
                "  -h, --help     Show this help message\n");
        return 0;
    }
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
