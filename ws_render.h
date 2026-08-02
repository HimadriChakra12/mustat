#ifndef WS_RENDER_H
#define WS_RENDER_H

#include "draw.h"
#include "workspace.h"

/* Total pixel width the workspace segment will occupy, per config.h's
 * ws_style. */
int ws_render_width(Draw *d, const WsItem *items, int count);

/* Draws the workspace segment starting at x (text baseline text_y,
 * full bar_height available for boxes/underlines). Returns the x
 * position just past the drawn segment. */
int ws_render_draw(Draw *d, int x, int text_y, int bar_height,
                    const WsItem *items, int count);

#endif
