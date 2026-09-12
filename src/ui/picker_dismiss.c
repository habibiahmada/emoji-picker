#include "ui/picker_dismiss.h"

#include "platform/x11_util.h"

#include <gdk/gdkx.h>
#include <X11/keysym.h>

void picker_hide(EmojiPicker *p) {
    if (p->dismiss_poll) {
        g_source_remove(p->dismiss_poll);
        p->dismiss_poll = 0;
    }
    if (p->finish_insert_timeout) {
        g_source_remove(p->finish_insert_timeout);
        p->finish_insert_timeout = 0;
    }
    if (p->focus_out_timeout) {
        g_source_remove(p->focus_out_timeout);
        p->focus_out_timeout = 0;
    }
    p->suppress_hide = FALSE;
    gtk_window_set_accept_focus(GTK_WINDOW(p->win), TRUE);
    gtk_widget_hide(p->win);
}

/* Close on click-outside, Esc, or Alt+Tab away from target.
 * NEVER dismiss on keypress while focus is on the target — that runs during
 * Ctrl+V and freezes Chromium mid-SelectionRequest.
 */
static gboolean dismiss_poll_cb(gpointer user_data) {
    EmojiPicker *p = user_data;
    if (!gtk_widget_get_visible(p->win)) {
        p->dismiss_poll = 0;
        return G_SOURCE_REMOVE;
    }
    if (p->suppress_hide)
        return G_SOURCE_CONTINUE;

    GdkDisplay *gdpy = gtk_widget_get_display(p->win);
    if (!gdpy || !GDK_IS_X11_DISPLAY(gdpy))
        return G_SOURCE_CONTINUE;
    Display *dpy = gdk_x11_display_get_xdisplay(gdpy);
    uint32_t self = x11_widget_xid(p->win);

    Window root_ret, child;
    int root_x = 0, root_y = 0, win_x = 0, win_y = 0;
    unsigned int mask = 0;
    if (XQueryPointer(dpy, DefaultRootWindow(dpy), &root_ret, &child, &root_x,
                      &root_y, &win_x, &win_y, &mask)) {
        if (mask & Button1Mask) {
            int x = 0, y = 0, w = 0, h = 0;
            gtk_window_get_position(GTK_WINDOW(p->win), &x, &y);
            gtk_window_get_size(GTK_WINDOW(p->win), &w, &h);
            if (root_x < x || root_y < y || root_x >= x + w || root_y >= y + h) {
                if (!p->outside_btn_latched) {
                    p->outside_btn_latched = TRUE;
                    picker_hide(p);
                    return G_SOURCE_REMOVE;
                }
            }
        } else {
            p->outside_btn_latched = FALSE;
        }
    }

    KeyCode esc = XKeysymToKeycode(dpy, XK_Escape);
    if (esc != 0) {
        char km[32];
        XQueryKeymap(dpy, km);
        int bi = esc / 8;
        int bit = 1 << (esc % 8);
        if (km[bi] & bit) {
            picker_hide(p);
            return G_SOURCE_REMOVE;
        }
    }

    uint32_t focus = x11_capture_active_xid();
    uint32_t focus_top = x11_toplevel_xid(dpy, focus);
    uint32_t self_top = x11_toplevel_xid(dpy, self);
    uint32_t sticky_top = x11_toplevel_xid(dpy, p->sticky_target);

    if (focus_top != 0 && self_top != 0 && focus_top != self_top &&
        (sticky_top == 0 || focus_top != sticky_top)) {
        picker_hide(p);
        return G_SOURCE_REMOVE;
    }

    return G_SOURCE_CONTINUE;
}

void picker_dismiss_poll_start(EmojiPicker *p) {
    p->keymap_ready = FALSE;
    p->outside_btn_latched = FALSE;
    if (p->dismiss_poll)
        return;
    p->dismiss_poll = g_timeout_add(DISMISS_POLL_MS, dismiss_poll_cb, p);
}
