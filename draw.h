#ifndef DRAW_H
#define DRAW_H

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

typedef struct {
    Display *dpy;
    Window win;

    XftDraw *draw;

    XftFont *font;

    XftColor fg;
    XftColor bg;

} Draw;

void draw_init(
    Draw *d,
    Display *dpy,
    Window win,
    const char *fontname,
    const char *fg,
    const char *bg
);

void draw_rect(Draw *d, int x, int y, int w, int h);

void draw_text(Draw *d, int x, int y, const char *text);

int text_width(Draw *d, const char *text);

#endif
