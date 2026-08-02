#ifndef TRAY_H
#define TRAY_H

#include <X11/Xlib.h>

#define TRAY_MAX_ICONS 16

typedef struct {
    Display *dpy;
    Window   win;              /* bar window: selection owner + embed parent */
    Atom     a_manager;
    Atom     a_opcode;
    Atom     a_xembed;
    Atom     a_xembed_info;
    Atom     a_selection;      /* _NET_SYSTEM_TRAY_S<n> */

    int      collapsed;        /* hardcoded feature: collapsed vs expanded  */
    Window   icons[TRAY_MAX_ICONS];
    int      icon_count;
} Tray;

/* Claims the systray manager selection on `win`. Safe to call even if
 * another tray manager is already running (dock requests just won't
 * arrive in that case). */
void tray_init(Tray *t, Display *dpy, Window win, int screen);

/* Feed every ClientMessage seen on `win`. Returns 1 if it was a dock
 * request and was handled (caller should mark the bar dirty). */
int  tray_handle_client_message(Tray *t, const XClientMessageEvent *cm);

/* Feed DestroyNotify/UnmapNotify for any window; drops it if it was a
 * tracked icon. Returns 1 if something changed. */
int  tray_handle_structure_event(Tray *t, Window w, int destroyed);

/* Toggle collapsed/expanded (call on tray-toggle click). */
void tray_toggle(Tray *t);

/* Total pixel width of the tray region (icons when expanded, just the
 * toggle glyph when collapsed), given icon geometry. */
int  tray_width(const Tray *t, int icon_size, int gap, int glyph_w, int pad);

/* Moves/maps/unmaps icon windows so the region starts at `x` (left
 * edge of the tray region, icons only — toggle glyph is drawn by the
 * caller) and returns the x where the toggle glyph should be drawn. */
int  tray_layout(Tray *t, int x, int icon_size, int gap, int bar_height);

#endif
