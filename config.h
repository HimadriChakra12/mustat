#ifndef CONFIG_H
#define CONFIG_H

typedef enum { WS_STYLE_BLOCK, WS_STYLE_BRACKET, WS_STYLE_UNDERLINE } WsStyle;
typedef enum { WS_POS_LEFT, WS_POS_CENTER, WS_POS_RIGHT } WsPos;

/* ── appearance ───────────────────────────────────────────── */
static const char *font       = "JetBrainsMono Nerd Font:size=10";
static const char *bg         = "#1d2021";
static const char *fg         = "#e2d2ab";
static const char *sep_color  = "#333333"; /* crisp 1px separators between groups */

static int padding            = 15;
static int bar_height         = 32;
static int margin             = 0;
static int sep_pad            = 10; /* space either side of a separator line */

/* ── systray ──────────────────────────────────────────────── */
static int tray_icon_size     = 20;
static int tray_icon_gap      = 6;
static int tray_pad           = 12; /* space between tray icons and separator */

static WsStyle ws_style = WS_STYLE_UNDERLINE;
static WsPos   ws_pos   = WS_POS_LEFT;

// index i = label for workspace i+1. NULL or "" falls back to the plain number, so you only need to name the ones you use.
static const char *ws_names[] = {
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
};
static int ws_name_count = sizeof(ws_names) / sizeof(ws_names[0]);

static int ws_gap             = 9;  /* spacing between workspace items       */
static int ws_inset           = 8;  /* vertical inset for boxes/underline    */

/* WS_STYLE_BLOCK only */
static const char *ws_block_bg        = "#1a1a1a";
static const char *ws_block_active_bg = "#ffffff";
static const char *ws_block_active_fg = "#000000";
static int ws_block_pad       = 5;

/* WS_STYLE_UNDERLINE only */
static const char *ws_underline_color = "#ffffff";
static int ws_underline_h     = 2;

#endif
