#include "bar.h"
#include "draw.h"
#include "module.h"
#include "config.h"

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <unistd.h>
#include <string.h>

void bar_run()
{
    Display *dpy = XOpenDisplay(NULL);
    int screen = DefaultScreen(dpy);

    int width = DisplayWidth(dpy,screen);

    Window root = RootWindow(dpy,screen);

    XSetWindowAttributes attr;
    attr.override_redirect = True;

    Window win = XCreateWindow(
        dpy,
        root,
        0,
        0,
        width,
        bar_height,
        0,
        DefaultDepth(dpy,screen),
        CopyFromParent,
        DefaultVisual(dpy,screen),
        CWOverrideRedirect,
        &attr
    );

    /* make it a DOCK window */

    Atom dock = XInternAtom(dpy,"_NET_WM_WINDOW_TYPE_DOCK",False);
    Atom type = XInternAtom(dpy,"_NET_WM_WINDOW_TYPE",False);

    XChangeProperty(
        dpy,
        win,
        type,
        XA_ATOM,
        32,
        PropModeReplace,
        (unsigned char*)&dock,
        1
    );

    XMapWindow(dpy,win);

    Draw draw;
    draw_init(&draw, dpy, win, font, fg, bg);
    // Add this once before the while loop, after draw_init:
    int text_y = (bar_height + draw.font->ascent - draw.font->descent) / 2;

    while(1)
    {
        XClearWindow(dpy, win);
        draw_rect(&draw, 0, 0, width, bar_height);  // fill bg with your config color

        /* LEFT MODULES */

        int x = padding;

        for(int i=0;i<left_count;i++)
        {
            module_run(&left_modules[i]);

            draw_text(&draw, x, text_y, left_modules[i].output);

            x += text_width(&draw,left_modules[i].output) + padding;
        }

        /* RIGHT MODULES */

        int rx = width - padding;

        for(int i=right_count-1;i>=0;i--)
        {
            module_run(&right_modules[i]);

            int w = text_width(&draw,right_modules[i].output);

            rx -= w;

            draw_text(&draw, rx, text_y, right_modules[i].output);

            rx -= padding;
        }

        /* CENTER MODULES */

        int center_width = 0;

        /* update modules and compute total width */

        for(int i=0;i<center_count;i++)
        {
            module_run(&center_modules[i]);

            int w = text_width(&draw, center_modules[i].output);
            center_width += w;

            if(i < center_count - 1)
                center_width += padding;
        }

        /* calculate starting X for centered block */

        int cx = (width - center_width) / 2;

        /* draw modules */

        for(int i=0;i<center_count;i++)
        {

            draw_text(&draw, cx, text_y, center_modules[i].output);

            cx += text_width(&draw, center_modules[i].output) + padding;
        }

        XFlush(dpy);

        sleep(1);
    }
}
