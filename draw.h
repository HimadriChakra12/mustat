#ifndef DRAW_H
#define DRAW_H

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

typedef struct {
    Display *dpy;
    Window win;
    XftDraw *draw;
    XftFont *font;
    XftColor color;
} Draw;

void draw_init(Draw *d, Display *dpy, Window win);
void draw_text(Draw *d, int x, int y, const char *text);

#endif
