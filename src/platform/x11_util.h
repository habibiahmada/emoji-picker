#ifndef X11_UTIL_H
#define X11_UTIL_H

#include <stdint.h>

#include <gtk/gtk.h>
#include <X11/Xlib.h>

uint32_t x11_toplevel_xid(Display *dpy, uint32_t xid);
uint32_t x11_capture_active_xid(void);
uint32_t x11_widget_xid(GtkWidget *w);

#endif
