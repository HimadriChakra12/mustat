#define _POSIX_C_SOURCE 200809L

#include "bar.h"
#include "draw.h"
#include "i3ipc.h"
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
    int  slot;
    Pos  pos;
    char text[256];
} Block;

static Block blocks[MAX_BLOCKS];
static int   block_count = 0;

static void block_update(Pos pos, int slot, const char *text)
{
    for (int i = 0; i < block_count; i++) {
        if (blocks[i].pos == slot && blocks[i].slot == slot) {
            snprintf(blocks[i].text, sizeof(blocks[i].text), "%s", text);
            return;
        }
    }
    if (block_count < MAX_BLOCKS) {
        blocks[block_count].pos  = pos;
        blocks[block_count].slot = slot;
        snprintf(blocks[block_count].text, sizeof(blocks[block_count].text), "%s", text);
        block_count++;
    }
}

/* parse "LEFT:1:some text" */
static void parse_line(char *line)
{
    Pos pos;
    if      (strncmp(line, "LEFT:",   5) == 0) { pos = POS_LEFT;   line += 5; }
    else if (strncmp(line, "CENTER:", 7) == 0) { pos = POS_CENTER; line += 7; }
    else if (strncmp(line, "RIGHT:",  6) == 0) { pos = POS_RIGHT;  line += 6; }
    else return;

    int slot = atoi(line);
    char *colon = strchr(line, ':');
    if (!colon) return;
    block_update(pos, slot, colon + 1);
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

static void redraw(Draw *draw, int bar_width, int text_y, char *ws_buf)
{
    draw_rect(draw, 0, 0, bar_width, bar_height);

    /* LEFT — workspace always first */
    int x = padding;
    draw_text(draw, x, text_y, ws_buf);
    x += text_width(draw, ws_buf) + padding;

    for (int i = 0; i < block_count; i++) {
        if (blocks[i].pos != POS_LEFT) continue;
        draw_text(draw, x, text_y, blocks[i].text);
        x += text_width(draw, blocks[i].text) + padding;
    }

    /* RIGHT */
    int rx = bar_width - padding;
    for (int i = block_count - 1; i >= 0; i--) {
        if (blocks[i].pos != POS_RIGHT) continue;
        int w = text_width(draw, blocks[i].text);
        rx -= w;
        draw_text(draw, rx, text_y, blocks[i].text);
        rx -= padding;
    }

    /* CENTER */
    int cw = blocks_total_width(draw, POS_CENTER);
    int cx = (bar_width - cw) / 2;
    for (int i = 0; i < block_count; i++) {
        if (blocks[i].pos != POS_CENTER) continue;
        draw_text(draw, cx, text_y, blocks[i].text);
        cx += text_width(draw, blocks[i].text) + padding;
    }
}

static int server_init(void)
{
    unlink(IBAR_SOCKET);
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    fcntl(fd, F_SETFL, O_NONBLOCK);

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", IBAR_SOCKET);

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
    attr.override_redirect = True;

    Window win = XCreateWindow(
        dpy, root,
        margin, margin,
        width - 2*margin, bar_height,
        0,
        DefaultDepth(dpy, screen),
        CopyFromParent,
        DefaultVisual(dpy, screen),
        CWOverrideRedirect,
        &attr
    );

    XStoreName(dpy, win, "ibar");

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
    draw_init(&draw, dpy, win, font, fg, bg);
    int bar_width = width - 2*margin;
    int text_y    = (bar_height + draw.font->ascent - draw.font->descent) / 2;

    int i3_fd  = i3_subscribe_workspaces();
    int srv_fd = server_init();

    char ws_buf[256];
    i3_get_workspaces(-1, ws_buf, sizeof(ws_buf));

    redraw(&draw, bar_width, text_y, ws_buf);
    XFlush(dpy);

    /* connected iblocks clients */
    int clients[8];
    int client_count = 0;
    memset(clients, -1, sizeof(clients));

    char linebuf[512];

    while (1)
    {
        fd_set fds;
        FD_ZERO(&fds);
        if (i3_fd  >= 0) FD_SET(i3_fd,  &fds);
        if (srv_fd >= 0) FD_SET(srv_fd, &fds);

        int maxfd = (i3_fd > srv_fd ? i3_fd : srv_fd);

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
            /* new iblocks connection */
            if (srv_fd >= 0 && FD_ISSET(srv_fd, &fds)) {
                int cfd = accept(srv_fd, NULL, NULL);
                if (cfd >= 0 && client_count < 8)
                    clients[client_count++] = cfd;
            }

            /* data from iblocks */
            for (int i = 0; i < client_count; i++) {
                if (clients[i] < 0 || !FD_ISSET(clients[i], &fds)) continue;
                int n = read(clients[i], linebuf, sizeof(linebuf) - 1);
                if (n <= 0) {
                    close(clients[i]);
                    clients[i] = -1;
                } else {
                    linebuf[n] = 0;
                    /* split on newlines */
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

            /* i3 workspace event */
            if (i3_fd >= 0 && FD_ISSET(i3_fd, &fds)) {
                i3_drain_event(i3_fd);
                i3_get_workspaces(-1, ws_buf, sizeof(ws_buf));
                dirty = 1;
            }
        } else {
            /* 1s tick */
            dirty = 1;
        }

        if (dirty) {
            redraw(&draw, bar_width, text_y, ws_buf);
            XFlush(dpy);
        }
    }
}
