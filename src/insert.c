#include "insert.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static uint32_t g_target_xid;
static uint32_t g_picker_xid;

void emoji_insert_set_target(uint32_t xid) {
    g_target_xid = xid;
}

void emoji_insert_set_picker(uint32_t xid) {
    g_picker_xid = xid;
}

/* Type glyph into the target window (no clipboard). Then restore picker focus. */
static void try_type(const char *glyph) {
    pid_t pid = fork();
    if (pid != 0)
        return;

    char target[32];
    char picker[32];
    snprintf(target, sizeof(target), "%u", (unsigned)g_target_xid);
    snprintf(picker, sizeof(picker), "%u", (unsigned)g_picker_xid);

    if (g_target_xid != 0 && g_picker_xid != 0) {
        execlp("xdotool", "xdotool", "windowactivate", "--sync", target, "type",
               "--clearmodifiers", "--delay", "0", "--", glyph, "windowactivate",
               "--sync", picker, (char *)NULL);
    } else if (g_target_xid != 0) {
        execlp("xdotool", "xdotool", "windowactivate", "--sync", target, "type",
               "--clearmodifiers", "--delay", "0", "--", glyph, (char *)NULL);
    } else {
        execlp("xdotool", "xdotool", "type", "--clearmodifiers", "--delay", "0",
               "--", glyph, (char *)NULL);
    }
    _exit(127);
}

void emoji_insert(const char *glyph) {
    if (!glyph || !glyph[0])
        return;
    try_type(glyph);
}
