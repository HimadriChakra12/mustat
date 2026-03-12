#ifndef CONFIG_H
#define CONFIG_H
#include "module.h"

static const char *font   = "JetBrainsMono Nerd Font:size=10";
static const char *fg     = "#282828";
static const char *bg     = "#ffffff";
static int padding        = 20;
static int bar_height     = 42;
static int margin = 10;  /* pixels — set to 0 to disable */

/* LEFT — workspace is handled natively in bar.c via i3ipc, no module needed */
static Module left_modules[]   = { };
static int    left_count       = 0;

/* CENTER */
static Module center_modules[] = {
    { MOD_CLOCK, NULL, "" }
};
static int center_count = sizeof(center_modules) / sizeof(Module);

/* RIGHT */
static Module right_modules[] = {
    { MOD_MEM, NULL, "" },
    { MOD_CPU, NULL, "" }
};
static int right_count = sizeof(right_modules) / sizeof(Module);

#endif
