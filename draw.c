#include "draw.h"
#include <string.h>

void draw_init(
    Draw *d,
    Display *dpy,
    Window win,
    const char *fontname,
    const char *fg,
    const char *bg
)
{
    int screen = DefaultScreen(dpy);

    d->dpy = dpy;
    d->win = win;

    d->draw = XftDrawCreate(
        dpy,
        win,
        DefaultVisual(dpy,screen),
        DefaultColormap(dpy,screen)
    );

    d->font = XftFontOpenName(
        dpy,
        screen,
        fontname
    );

    XftColorAllocName(
        dpy,
        DefaultVisual(dpy,screen),
        DefaultColormap(dpy,screen),
        fg,
        &d->fg
    );

    XftColorAllocName(
        dpy,
        DefaultVisual(dpy,screen),
        DefaultColormap(dpy,screen),
        bg,
        &d->bg
    );
}

void draw_rect(Draw *d, int x, int y, int w, int h)
{
    XftDrawRect(
        d->draw,
        &d->bg,
        x,
        y,
        w,
        h
    );
}

void draw_text(Draw *d, int x, int y, const char *text)
{
    XftDrawStringUtf8(
        d->draw,
        &d->fg,
        d->font,
        x,
        y,
        (XftChar8*)text,
        strlen(text)
    );
}

int text_width(Draw *d, const char *text)
{
    XGlyphInfo ext;

    XftTextExtentsUtf8(
        d->dpy,
        d->font,
        (XftChar8*)text,
        strlen(text),
        &ext
    );

    return ext.xOff;
}
