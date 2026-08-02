#define _POSIX_C_SOURCE 200809L

#include "bar.h"
#include "draw.h"
#include "workspace.h"
#include "ws_render.h"
#include "tray.h"
#include "ipc.h"
#include "config.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>

#define MAX_BLOCKS 32

typedef enum { POS_LEFT, POS_CENTER, POS_RIGHT } Pos;

typedef struct {
    Pos  pos;
    char name[64];
    char text[256];
} Block;

static Block blocks[MAX_BLOCKS];
static int   block_count = 0;

static void block_update(Pos pos, const char *name, const char *text)
{
    for (int i = 0; i < block_count; i++) {
        if (blocks[i].pos == pos && strcmp(blocks[i].name, name) == 0) {
            snprintf(blocks[i].text, sizeof(blocks[i].text), "%s", text);
            return;
        }
    }
    if (block_count < MAX_BLOCKS) {
        blocks[block_count].pos = pos;
        snprintf(blocks[block_count].name, sizeof(blocks[block_count].name), "%s", name);
        snprintf(blocks[block_count].text, sizeof(blocks[block_count].text), "%s", text);
        block_count++;
    }
}

/* parse "LEFT:clock: 12:30:00" — pos, then name (no colon), then text
 * (which may itself contain colons, e.g. clock output). */
static void parse_line(char *line)
{
    Pos pos;
    if      (strncmp(line, "LEFT:",   5) == 0) { pos = POS_LEFT;   line += 5; }
    else if (strncmp(line, "CENTER:", 7) == 0) { pos = POS_CENTER; line += 7; }
    else if (strncmp(line, "RIGHT:",  6) == 0) { pos = POS_RIGHT;  line += 6; }
    else return;

    char *colon = strchr(line, ':');
    if (!colon) return;
    *colon = 0;
    block_update(pos, line, colon + 1);
}

static int blocks_total_width(Draw *draw, Pos pos)
{
    int w = 0;
    for (int i = 0; i < block_count; i++) {
        if (blocks[i].pos != pos) continue;
        w += text_width(draw, blocks[i].text) + padding;
    }
    return w;
}

static void redraw(Draw *draw, Tray *tray, int bar_width, int text_y,
                    WsItem *ws_items, int ws_count)
{
    draw_rect(draw, 0, 0, bar_width, bar_height);

    int ws_w = (ws_count > 0) ? ws_render_width(draw, ws_items, ws_count) : 0;

    /* ── tray region, reserved from the right edge first ── */
    int rx = bar_width - padding;

    if (tray->icon_count > 0) {
        int icons_w = tray_width(tray, tray_icon_size, tray_icon_gap);
        rx -= icons_w;
        tray_layout(tray, rx, tray_icon_size, tray_icon_gap, bar_height);
        rx -= sep_pad / 2;
        draw_vline(draw, rx, 8, bar_height);
        rx -= sep_pad / 2;
    }

    /* ── LEFT: workspace (if positioned here) then LEFT blocks ── */
    int x = padding;
    if (ws_pos == WS_POS_LEFT && ws_count > 0) {
        x = ws_render_draw(draw, x, text_y, bar_height, ws_items, ws_count);
        x += sep_pad;
        draw_vline(draw, x, 8, bar_height);
        x += sep_pad;
    }
    for (int i = 0; i < block_count; i++) {
        if (blocks[i].pos != POS_LEFT) continue;
        draw_text(draw, x, text_y, blocks[i].text);
        x += text_width(draw, blocks[i].text) + padding;
    }

    /* ── RIGHT blocks (stopping short of the tray region), then
     *    the workspace segment if it's positioned here ── */
    for (int i = block_count - 1; i >= 0; i--) {
        if (blocks[i].pos != POS_RIGHT) continue;
        int w = text_width(draw, blocks[i].text);
        rx -= w;
        draw_text(draw, rx, text_y, blocks[i].text);
        rx -= padding;
    }
    if (ws_pos == WS_POS_RIGHT && ws_count > 0) {
        rx -= sep_pad;
        draw_vline(draw, rx, 8, bar_height);
        rx -= sep_pad;
        rx -= ws_w;
        ws_render_draw(draw, rx, text_y, bar_height, ws_items, ws_count);
    }

    /* ── CENTER: workspace (if positioned here) then CENTER blocks,
     *    the whole group centered as one unit ── */
    int cw = blocks_total_width(draw, POS_CENTER);
    int total_w = cw;
    if (ws_pos == WS_POS_CENTER && ws_count > 0)
        total_w += ws_w + 2 * sep_pad + 1;

    int cx = (bar_width - total_w) / 2;
    if (ws_pos == WS_POS_CENTER && ws_count > 0) {
        cx = ws_render_draw(draw, cx, text_y, bar_height, ws_items, ws_count);
        cx += sep_pad;
        draw_vline(draw, cx, 8, bar_height);
        cx += sep_pad;
    }
    for (int i = 0; i < block_count; i++) {
        if (blocks[i].pos != POS_CENTER) continue;
        draw_text(draw, cx, text_y, blocks[i].text);
        cx += text_width(draw, blocks[i].text) + padding;
    }
}

static int server_init(void)
{
    unlink(MUSTAT_SOCKET);
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    fcntl(fd, F_SETFL, O_NONBLOCK);

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", MUSTAT_SOCKET);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) { close(fd); return -1; }
    listen(fd, 4);
    return fd;
}

void bar_run(void)
{
    Display *dpy = XOpenDisplay(NULL);
    int screen   = DefaultScreen(dpy);
    int width    = DisplayWidth(dpy, screen);
    Window root  = RootWindow(dpy, screen);

    XSetWindowAttributes attr = {0};
    attr.event_mask = ExposureMask | StructureNotifyMask;

    Window win = XCreateWindow(
        dpy, root,
        margin, margin,
        width - 2*margin, bar_height,
        0,
        DefaultDepth(dpy, screen),
        CopyFromParent,
        DefaultVisual(dpy, screen),
        CWEventMask,
        &attr
    );

    XStoreName(dpy, win, "mustat");

    /* dock type */
    Atom dock = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);
    Atom type = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
    XChangeProperty(dpy, win, type, XA_ATOM, 32, PropModeReplace,
                    (unsigned char*)&dock, 1);

    /* strut */
    Atom strut_partial = XInternAtom(dpy, "_NET_WM_STRUT_PARTIAL", False);
    long struts[12] = {0};
    struts[2] = bar_height + margin;
    struts[8] = margin;
    struts[9] = width - margin;
    XChangeProperty(dpy, win, strut_partial, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char*)struts, 12);
    Atom strut = XInternAtom(dpy, "_NET_WM_STRUT", False);
    XChangeProperty(dpy, win, strut, XA_CARDINAL, 32,
                    PropModeReplace, (unsigned char*)struts, 4);

    XMapWindow(dpy, win);
    XRaiseWindow(dpy, win);

    Draw draw;
    int bar_width = width - 2*margin;
    draw_init(&draw, dpy, win, bar_width, bar_height, font, fg, bg, sep_color);
    int text_y    = (bar_height + draw.font->ascent - draw.font->descent) / 2;

    Workspace ws;
    ws_init(&ws, dpy, root);

    Tray tray;
    tray_init(&tray, dpy, win, screen);

    int srv_fd = server_init();

    WsItem ws_items[WS_MAX_ITEMS];
    int    ws_count = ws_get_items(&ws, ws_items, WS_MAX_ITEMS);

    redraw(&draw, &tray, bar_width, text_y, ws_items, ws_count);
    draw_present(&draw, bar_width, bar_height);
    XFlush(dpy);

    /* connected mublocks clients */
    int clients[8];
    int client_count = 0;
    memset(clients, -1, sizeof(clients));

    char linebuf[512];
    int xfd = ConnectionNumber(dpy);

    while (1)
    {
        fd_set fds;
        FD_ZERO(&fds);
        int ws_fd_ = ws_fd(&ws);
        if (ws_fd_ >= 0) FD_SET(ws_fd_, &fds);
        if (srv_fd >= 0) FD_SET(srv_fd, &fds);
        FD_SET(xfd, &fds);

        int maxfd = xfd;
        if (ws_fd_ > maxfd) maxfd = ws_fd_;
        if (srv_fd > maxfd) maxfd = srv_fd;

        for (int i = 0; i < client_count; i++) {
            if (clients[i] >= 0) {
                FD_SET(clients[i], &fds);
                if (clients[i] > maxfd) maxfd = clients[i];
            }
        }

        struct timeval tv = { .tv_sec = 1, .tv_usec = 0 };
        int ret = select(maxfd + 1, &fds, NULL, NULL, &tv);

        int dirty = 0;

        if (ret > 0) {
            /* new mublocks connection */
            if (srv_fd >= 0 && FD_ISSET(srv_fd, &fds)) {
                int cfd = accept(srv_fd, NULL, NULL);
                if (cfd >= 0 && client_count < 8)
                    clients[client_count++] = cfd;
            }

            /* data from mublocks */
            for (int i = 0; i < client_count; i++) {
                if (clients[i] < 0 || !FD_ISSET(clients[i], &fds)) continue;
                int n = read(clients[i], linebuf, sizeof(linebuf) - 1);
                if (n <= 0) {
                    close(clients[i]);
                    clients[i] = -1;
                } else {
                    linebuf[n] = 0;
                    char *line = linebuf;
                    char *nl;
                    while ((nl = strchr(line, '\n'))) {
                        *nl = 0;
                        if (*line) parse_line(line);
                        line = nl + 1;
                    }
                    dirty = 1;
                }
            }

            /* i3/sway workspace event */
            if (ws_fd_ >= 0 && FD_ISSET(ws_fd_, &fds)) {
                ws_drain_ipc(&ws);
                ws_count = ws_get_items(&ws, ws_items, WS_MAX_ITEMS);
                dirty = 1;
            }

            /* X11 events: EWMH workspace changes, tray protocol, clicks */
            if (FD_ISSET(xfd, &fds)) {
                XEvent ev;
                while (XPending(dpy)) {
                    XNextEvent(dpy, &ev);

                    if (ws_handle_xevent(&ws, &ev)) {
                        ws_count = ws_get_items(&ws, ws_items, WS_MAX_ITEMS);
                        dirty = 1;
                    }

                    switch (ev.type) {
                        case ClientMessage:
                            if (tray_handle_client_message(&tray, &ev.xclient))
                                dirty = 1;
                            break;
                        case DestroyNotify:
                            if (tray_handle_structure_event(&tray, ev.xdestroywindow.window, 1))
                                dirty = 1;
                            break;
                        case Expose:
                            dirty = 1;
                            break;
                        default:
                            break;
                    }
                }
            }
        } else {
            /* 1s tick */
            dirty = 1;
        }

        if (dirty) {
            redraw(&draw, &tray, bar_width, text_y, ws_items, ws_count);
            draw_present(&draw, bar_width, bar_height);
            XFlush(dpy);
        }
    }
}
