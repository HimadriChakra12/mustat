#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <unistd.h>
#include <stdio.h>

#include "draw.h"
#include "module.h"
#include "config.h"
#include "i3ipc.h"

void set_window_type_dock(Display *dpy, Window win, int height, int width)
{
    Atom type = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
    Atom dock = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);
    XChangeProperty(dpy, win, type, XA_ATOM, 32, PropModeReplace, (unsigned char *)&dock, 1);

    Atom strut = XInternAtom(dpy, "_NET_WM_STRUT_PARTIAL", False);
    long strut_data[12] = {0};
    strut_data[2] = height;   // top strut
    strut_data[8] = 0;
    strut_data[9] = width;
    XChangeProperty(dpy, win, strut, XA_CARDINAL, 32, PropModeReplace, (unsigned char*)strut_data, 12);
}

void bar_run()
{
    Display *dpy = XOpenDisplay(NULL);
    int screen = DefaultScreen(dpy);

    int width = DisplayWidth(dpy,screen);
    int height = 24;

    Window win = XCreateSimpleWindow(dpy, RootWindow(dpy,screen), 0,0,width,height,0,0,0);
    set_window_type_dock(dpy, win, height, width);
    XMapWindow(dpy, win);

    Draw draw;
    draw_init(&draw, dpy, win);

    char workspace[128];

    while(1)
    {
        XClearWindow(dpy, win);

        i3_get_workspaces(workspace, sizeof(workspace));
        draw_text(&draw, 10, 16, workspace);

        int x = 200;
        for(int i=0;i<sizeof(left_modules)/sizeof(Module);i++)
        {
            module_update(&left_modules[i]);
            draw_text(&draw,x,16,left_modules[i].output);
            x += 150;
        }

        int cx = width/2;
        for(int i=0;i<sizeof(center_modules)/sizeof(Module);i++)
        {
            module_update(&center_modules[i]);
            draw_text(&draw,cx,16,center_modules[i].output);
            cx += 150;
        }

        int rx = width - 300;
        for(int i=0;i<sizeof(right_modules)/sizeof(Module);i++)
        {
            module_update(&right_modules[i]);
            draw_text(&draw,rx,16,right_modules[i].output);
            rx += 150;
        }

        XFlush(dpy);
        sleep(1);
    }
}
