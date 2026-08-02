#include "draw.h"
#include <string.h>

void draw_init(
    Draw *d,
    Display *dpy,
    Window win,
    int w, int h,
    const char *fontname,
    const char *fg,
    const char *bg,
    const char *sep
)
{
    int screen = DefaultScreen(dpy);

    d->dpy    = dpy;
    d->win    = win;
    d->visual = DefaultVisual(dpy, screen);
    d->cmap   = DefaultColormap(dpy, screen);

    d->buf = XCreatePixmap(dpy, win, w, h, DefaultDepth(dpy, screen));
    d->gc  = XCreateGC(dpy, win, 0, NULL);

    d->draw = XftDrawCreate(dpy, d->buf, d->visual, d->cmap);
    d->font = XftFontOpenName(dpy, screen, fontname);

    XftColorAllocName(dpy, d->visual, d->cmap, fg,  &d->fg);
    XftColorAllocName(dpy, d->visual, d->cmap, bg,  &d->bg);
    XftColorAllocName(dpy, d->visual, d->cmap, sep, &d->sep);
}

void draw_present(Draw *d, int w, int h)
{
    XCopyArea(d->dpy, d->buf, d->win, d->gc, 0, 0, w, h, 0, 0);
}

void draw_rect(Draw *d, int x, int y, int w, int h)
{
    XftDrawRect(d->draw, &d->bg, x, y, w, h);
}

void draw_vline(Draw *d, int x, int inset, int bar_height)
{
    XftDrawRect(d->draw, &d->sep, x, inset, 1, bar_height - 2 * inset);
}

void draw_text(Draw *d, int x, int y, const char *text)
{
    XftDrawStringUtf8(d->draw, &d->fg, d->font, x, y, (XftChar8*)text, strlen(text));
}

int text_width(Draw *d, const char *text)
{
    XGlyphInfo ext;
    XftTextExtentsUtf8(d->dpy, d->font, (XftChar8*)text, strlen(text), &ext);
    return ext.xOff;
}

XftColor draw_color(Draw *d, const char *name)
{
    XftColor c;
    XftColorAllocName(d->dpy, d->visual, d->cmap, name, &c);
    return c;
}

void draw_rect_c(Draw *d, const XftColor *c, int x, int y, int w, int h)
{
    XftDrawRect(d->draw, c, x, y, w, h);
}

void draw_text_c(Draw *d, const XftColor *c, int x, int y, const char *text)
{
    XftDrawStringUtf8(d->draw, c, d->font, x, y, (XftChar8*)text, strlen(text));
}
