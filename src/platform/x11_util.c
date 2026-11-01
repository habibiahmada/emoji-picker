#include "platform/x11_util.h"

#include <X11/Xatom.h>
#include <gdk/gdkx.h>

/* Stale XIDs (target closed) raise BadWindow; trap so GDK does not abort. */
uint32_t x11_toplevel_xid(Display *dpy, uint32_t xid) {
    if (!dpy || xid == 0)
        return 0;
    GdkDisplay *gdpy = gdk_x11_lookup_xdisplay(dpy);
    if (gdpy)
        gdk_x11_display_error_trap_push(gdpy);
    Window w = (Window)xid;
    for (;;) {
        Window root = None, parent = None, *kids = NULL;
        unsigned n = 0;
        if (!XQueryTree(dpy, w, &root, &parent, &kids, &n))
            break;
        if (kids)
            XFree(kids);
        if (parent == None || parent == root)
            break;
        w = parent;
    }
    if (gdpy)
        gdk_x11_display_error_trap_pop_ignored(gdpy);
    return (uint32_t)w;
}

/*
 * Same source as `xdotool getactivewindow` (_NET_ACTIVE_WINDOW), read over
 * GDK's own connection. Called from the 50ms dismiss poll — never fork here.
 */
static Window net_active_window(GdkDisplay *gdpy, Display *dpy) {
    Atom prop = gdk_x11_get_xatom_by_name_for_display(gdpy, "_NET_ACTIVE_WINDOW");
    Atom type = None;
    int format = 0;
    unsigned long n = 0, after = 0;
    unsigned char *data = NULL;
    Window w = None;

    gdk_x11_display_error_trap_push(gdpy);
    int rc = XGetWindowProperty(dpy, DefaultRootWindow(dpy), prop, 0, 1, False,
                                XA_WINDOW, &type, &format, &n, &after, &data);
    gdk_x11_display_error_trap_pop_ignored(gdpy);
    if (rc == Success && data && type == XA_WINDOW && format == 32 && n == 1)
        w = (Window)((unsigned long *)data)[0];
    if (data)
        XFree(data);
    return w;
}

uint32_t x11_capture_active_xid(void) {
    GdkDisplay *gdpy = gdk_display_get_default();
    if (!gdpy || !GDK_IS_X11_DISPLAY(gdpy))
        return 0;
    Display *dpy = GDK_DISPLAY_XDISPLAY(gdpy);

    Window active = net_active_window(gdpy, dpy);
    if (active != None)
        return (uint32_t)active;

    Window focus = None;
    int revert = 0;
    XGetInputFocus(dpy, &focus, &revert);
    if (focus == None || focus == PointerRoot)
        return 0;
    return (uint32_t)focus;
}

uint32_t x11_widget_xid(GtkWidget *w) {
    GdkWindow *gw = gtk_widget_get_window(w);
    if (!gw || !GDK_IS_X11_WINDOW(gw))
        return 0;
    return (uint32_t)gdk_x11_window_get_xid(gw);
}
