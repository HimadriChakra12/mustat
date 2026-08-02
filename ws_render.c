#define _POSIX_C_SOURCE 200809L

#include "ws_render.h"
#include "config.h"

#include <stdio.h>
#include <string.h>

static const char *ws_label(int num, char *buf, size_t bufsz)
{
    if (num >= 1 && num <= ws_name_count && ws_names[num - 1] && ws_names[num - 1][0])
        snprintf(buf, bufsz, "%s", ws_names[num - 1]);
    else
        snprintf(buf, bufsz, "%d", num);
    return buf;
}

int ws_render_width(Draw *d, const WsItem *items, int count)
{
    int w = 0;
    char label[64];

    for (int i = 0; i < count; i++) {
        ws_label(items[i].num, label, sizeof(label));
        int lw = text_width(d, label);

        switch (ws_style) {
            case WS_STYLE_BLOCK:
                w += lw + 2 * ws_block_pad;
                break;
            case WS_STYLE_BRACKET: {
                char tmp[70];
                if (items[i].focused) snprintf(tmp, sizeof(tmp), "[%s]", label);
                else                  snprintf(tmp, sizeof(tmp), "%s",   label);
                w += text_width(d, tmp);
                break;
            }
            case WS_STYLE_UNDERLINE:
                w += lw;
                break;
        }
        if (i < count - 1) w += ws_gap;
    }
    return w;
}

int ws_render_draw(Draw *d, int x, int text_y, int bar_height,
                    const WsItem *items, int count)
{
    char label[64];

    XftColor block_bg        = draw_color(d, ws_block_bg);
    XftColor block_active_bg = draw_color(d, ws_block_active_bg);
    XftColor block_active_fg = draw_color(d, ws_block_active_fg);
    XftColor underline_c     = draw_color(d, ws_underline_color);

    for (int i = 0; i < count; i++) {
        ws_label(items[i].num, label, sizeof(label));
        int focused = items[i].focused;

        switch (ws_style) {
            case WS_STYLE_BLOCK: {
                int lw = text_width(d, label);
                int bw = lw + 2 * ws_block_pad;
                int bh = bar_height - 2 * ws_inset;
                draw_rect_c(d, focused ? &block_active_bg : &block_bg,
                            x, ws_inset, bw, bh);
                draw_text_c(d, focused ? &block_active_fg : &d->fg,
                            x + ws_block_pad, text_y, label);
                x += bw;
                break;
            }
            case WS_STYLE_BRACKET: {
                char tmp[70];
                if (focused) snprintf(tmp, sizeof(tmp), "[%s]", label);
                else         snprintf(tmp, sizeof(tmp), "%s",   label);
                draw_text(d, x, text_y, tmp);
                x += text_width(d, tmp);
                break;
            }
            case WS_STYLE_UNDERLINE: {
                int lw = text_width(d, label);
                draw_text(d, x, text_y, label);
                if (focused)
                    draw_rect_c(d, &underline_c, x, bar_height - ws_inset,
                                lw, ws_underline_h);
                x += lw;
                break;
            }
        }
        if (i < count - 1) x += ws_gap;
    }
    return x;
}
