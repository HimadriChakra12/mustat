#include "bar.h"
#include "draw.h"
#include "module.h"
#include "i3ipc.h"
#include "config.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include <time.h>

static void redraw(Draw *draw, int width, int text_y, char *ws_buf)
{
    draw_rect(draw, 0, 0, width, bar_height);

    /* LEFT — workspace (driven by i3 events) */
    int x = padding;
    draw_text(draw, x, text_y, ws_buf);
    x += text_width(draw, ws_buf) + padding;

    /* any extra left modules */
    for (int i = 0; i < left_count; i++) {
        module_run(&left_modules[i]);
        draw_text(draw, x, text_y, left_modules[i].output);
        x += text_width(draw, left_modules[i].output) + padding;
    }

    /* RIGHT */
    int rx = width - padding;
    for (int i = right_count - 1; i >= 0; i--) {
        module_run(&right_modules[i]);
        int w = text_width(draw, right_modules[i].output);
        rx -= w;
        draw_text(draw, rx, text_y, right_modules[i].output);
        rx -= padding;
    }

    /* CENTER */
    int center_width = 0;
    for (int i = 0; i < center_count; i++) {
        module_run(&center_modules[i]);
        center_width += text_width(draw, center_modules[i].output);
        if (i < center_count - 1) center_width += padding;
    }
    int cx = (width - center_width) / 2;
    for (int i = 0; i < center_count; i++) {
        draw_text(draw, cx, text_y, center_modules[i].output);
        cx += text_width(draw, center_modules[i].output) + padding;
    }
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
        dpy, root, 0, 0, width, bar_height, 0,
        DefaultDepth(dpy, screen), CopyFromParent,
        DefaultVisual(dpy, screen), CWOverrideRedirect, &attr
    );

    Atom dock = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);
    Atom type = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
    XChangeProperty(dpy, win, type, XA_ATOM, 32, PropModeReplace,
                    (unsigned char*)&dock, 1);

    XMapWindow(dpy, win);

    Draw draw;
    draw_init(&draw, dpy, win, font, fg, bg);
    int text_y = (bar_height + draw.font->ascent - draw.font->descent) / 2;

    /* persistent i3 subscription socket */
    int i3_fd = i3_subscribe_workspaces();

    /* initial workspace fetch */
    char ws_buf[256];
    i3_get_workspaces(-1, ws_buf, sizeof(ws_buf));

    /* initial draw */
    redraw(&draw, width, text_y, ws_buf);
    XFlush(dpy);

    while (1)
    {
        /* wake up on i3 event OR every 1 second for clock/cpu/mem */
        fd_set fds;
        FD_ZERO(&fds);
        if (i3_fd >= 0) FD_SET(i3_fd, &fds);

        struct timeval tv = { .tv_sec = 1, .tv_usec = 0 };
        int ret = select(i3_fd + 1, &fds, NULL, NULL, &tv);

        if (ret > 0 && i3_fd >= 0 && FD_ISSET(i3_fd, &fds)) {
            /* workspace changed — drain event, re-fetch */
            i3_drain_event(i3_fd);
            i3_get_workspaces(-1, ws_buf, sizeof(ws_buf));
        }
        /* always redraw (updates clock/cpu/mem on timer, workspace on event) */
        redraw(&draw, width, text_y, ws_buf);
        XFlush(dpy);
    }
}
