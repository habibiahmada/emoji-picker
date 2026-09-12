#include "platform/x11_util.h"

#include <stdio.h>

#include <gdk/gdkx.h>

uint32_t x11_toplevel_xid(Display *dpy, uint32_t xid) {
    if (!dpy || xid == 0)
        return 0;
    Window w = (Window)xid;
    for (;;) {
        Window root = None, parent = None, *kids = NULL;
        unsigned n = 0;
        if (!XQueryTree(dpy, w, &root, &parent, &kids, &n))
            return (uint32_t)w;
        if (kids)
            XFree(kids);
        if (parent == None || parent == root)
            return (uint32_t)w;
        w = parent;
    }
}

uint32_t x11_capture_active_xid(void) {
    FILE *fp = popen("xdotool getactivewindow 2>/dev/null", "r");
    if (fp) {
        unsigned long xid = 0;
        if (fscanf(fp, "%lu", &xid) == 1 && xid != 0) {
            pclose(fp);
            return (uint32_t)xid;
        }
        pclose(fp);
    }

    GdkDisplay *gdpy = gdk_display_get_default();
    if (!GDK_IS_X11_DISPLAY(gdpy))
        return 0;
    Display *dpy = GDK_DISPLAY_XDISPLAY(gdpy);
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
