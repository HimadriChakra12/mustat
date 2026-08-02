#ifndef WORKSPACE_H
#define WORKSPACE_H

#include <X11/Xlib.h>

typedef enum { WS_NONE, WS_IPC, WS_EWMH } WsBackend;

#define WS_MAX_ITEMS 32

typedef struct {
    int num;     /* 1-indexed workspace number, used for ws_names[] lookup */
    int focused;
} WsItem;

typedef struct {
    WsBackend backend;
    int       ipc_fd;   /* i3/sway IPC event socket, -1 when using EWMH   */
    Display  *dpy;
    Window    root;
    Atom      a_current_desktop;
    Atom      a_num_desktops;
} Workspace;

/* Detects i3, sway, or falls back to plain EWMH (bspwm/openbox/dwm+ewmh/…). */
void ws_init(Workspace *ws, Display *dpy, Window root);

/* fd to add to select(), or -1 if this backend has none of its own
 * (EWMH rides on the existing X connection instead). */
int  ws_fd(const Workspace *ws);

/* Call when ws_fd(ws) is readable. */
void ws_drain_ipc(Workspace *ws);

/* Call for every XEvent pulled off the X connection; returns 1 if it
 * was a workspace-change event (EWMH backend only). */
int  ws_handle_xevent(Workspace *ws, XEvent *ev);

/* Fills `out` (capacity `max`) with the current workspace list in
 * order, 1-indexed. Returns the item count. */
int  ws_get_items(Workspace *ws, WsItem *out, int max);

#endif
