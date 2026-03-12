#include "draw.h"
#include <X11/Xft/Xft.h>
#include <string.h>

void draw_init(Draw *d, Display *dpy, Window win)
{
    int screen = DefaultScreen(dpy);
    d->dpy = dpy;
    d->win = win;
    d->draw = XftDrawCreate(dpy, win, DefaultVisual(dpy,screen), DefaultColormap(dpy,screen));
    d->font = XftFontOpenName(dpy, screen, "JetBrainsMono Nerd Font:size=10");

    XRenderColor xr = {65535,65535,65535,65535};
    XftColorAllocValue(dpy, DefaultVisual(dpy,screen), DefaultColormap(dpy,screen), &xr, &d->color);
}

void draw_text(Draw *d, int x, int y, const char *text)
{
    XftDrawStringUtf8(d->draw, &d->color, d->font, x, y, (XftChar8*)text, strlen(text));
}
