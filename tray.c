#define _POSIX_C_SOURCE 200809L

#include "tray.h"
#include <X11/Xatom.h>
#include <string.h>
#include <stdio.h>

#define SYSTEM_TRAY_REQUEST_DOCK 0
#define XEMBED_EMBEDDED_NOTIFY   0
#define XEMBED_MAPPED            (1 << 0)

void tray_init(Tray *t, Display *dpy, Window win, int screen)
{
    memset(t, 0, sizeof(*t));
    t->dpy = dpy;
    t->win = win;

    char sel_name[32];
    snprintf(sel_name, sizeof(sel_name), "_NET_SYSTEM_TRAY_S%d", screen);

    t->a_selection    = XInternAtom(dpy, sel_name, False);
    t->a_manager      = XInternAtom(dpy, "MANAGER", False);
    t->a_opcode       = XInternAtom(dpy, "_NET_SYSTEM_TRAY_OPCODE", False);
    t->a_xembed       = XInternAtom(dpy, "_XEMBED", False);
    t->a_xembed_info  = XInternAtom(dpy, "_XEMBED_INFO", False);

    XSetSelectionOwner(dpy, t->a_selection, win, CurrentTime);

    if (XGetSelectionOwner(dpy, t->a_selection) == win) {
        Window root = DefaultRootWindow(dpy);
        XClientMessageEvent ev = {0};
        ev.type         = ClientMessage;
        ev.window        = root;
        ev.message_type  = t->a_manager;
        ev.format        = 32;
        ev.data.l[0]     = CurrentTime;
        ev.data.l[1]     = t->a_selection;
        ev.data.l[2]     = win;
        XSendEvent(dpy, root, False, StructureNotifyMask, (XEvent*)&ev);
    }
    /* If another tray manager already owns the selection, we simply
     * never receive dock requests — no error, tray region stays empty. */
}

static void embed_icon(Tray *t, Window icon)
{
    if (t->icon_count >= TRAY_MAX_ICONS) return;

    XSelectInput(t->dpy, icon, StructureNotifyMask | PropertyChangeMask);
    XReparentWindow(t->dpy, icon, t->win, 0, 0);

    XClientMessageEvent note = {0};
    note.type        = ClientMessage;
    note.window       = icon;
    note.message_type = t->a_xembed;
    note.format       = 32;
    note.data.l[0]    = CurrentTime;
    note.data.l[1]    = XEMBED_EMBEDDED_NOTIFY;
    note.data.l[2]    = 0;
    note.data.l[3]    = t->win;
    note.data.l[4]    = 0;
    XSendEvent(t->dpy, icon, False, NoEventMask, (XEvent*)&note);

    t->icons[t->icon_count++] = icon;
}

int tray_handle_client_message(Tray *t, const XClientMessageEvent *cm)
{
    if (cm->message_type != t->a_opcode) return 0;
    if (cm->data.l[1] != SYSTEM_TRAY_REQUEST_DOCK) return 0;

    Window icon = (Window)cm->data.l[2];
    if (!icon) return 0;

    embed_icon(t, icon);
    return 1;
}

int tray_handle_structure_event(Tray *t, Window w, int destroyed)
{
    (void)destroyed;
    for (int i = 0; i < t->icon_count; i++) {
        if (t->icons[i] != w) continue;
        for (int j = i; j < t->icon_count - 1; j++)
            t->icons[j] = t->icons[j + 1];
        t->icon_count--;
        return 1;
    }
    return 0;
}

void tray_toggle(Tray *t)
{
    t->collapsed = !t->collapsed;
}

int tray_width(const Tray *t, int icon_size, int gap, int glyph_w, int pad)
{
    int w = glyph_w + pad;
    if (!t->collapsed && t->icon_count > 0)
        w += t->icon_count * (icon_size + gap);
    return w;
}

int tray_layout(Tray *t, int x, int icon_size, int gap, int bar_height)
{
    int y = (bar_height - icon_size) / 2;

    if (t->collapsed) {
        for (int i = 0; i < t->icon_count; i++)
            XUnmapWindow(t->dpy, t->icons[i]);
        return x;
    }

    int cx = x;
    for (int i = 0; i < t->icon_count; i++) {
        XMoveResizeWindow(t->dpy, t->icons[i], cx, y, icon_size, icon_size);
        XMapWindow(t->dpy, t->icons[i]);
        cx += icon_size + gap;
    }
    return cx;
}
