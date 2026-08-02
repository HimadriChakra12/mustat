#ifndef DRAW_H
#define DRAW_H

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>

typedef struct {
    Display *dpy;
    Window win;
    Visual  *visual;
    Colormap cmap;

    XftDraw *draw;
    XftFont *font;

    XftColor fg;
    XftColor bg;
    XftColor sep;

} Draw;

void draw_init(
    Draw *d,
    Display *dpy,
    Window win,
    const char *fontname,
    const char *fg,
    const char *bg,
    const char *sep
);

void draw_rect(Draw *d, int x, int y, int w, int h);

/* crisp 1px vertical divider, vertically inset by `inset` px top/bottom */
void draw_vline(Draw *d, int x, int inset, int bar_height);

void draw_text(Draw *d, int x, int y, const char *text);

int text_width(Draw *d, const char *text);

/* ── arbitrary-color variants, used for workspace styling ──────────── */

/* Allocates an XftColor by name (e.g. "#ffffff"). Caller owns the
 * result; there is no free() call needed for the lifetime of a bar
 * process, colors are released with the colormap on exit. */
XftColor draw_color(Draw *d, const char *name);

void draw_rect_c(Draw *d, const XftColor *c, int x, int y, int w, int h);
void draw_text_c(Draw *d, const XftColor *c, int x, int y, const char *text);

#endif
