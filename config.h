#ifndef CONFIG_H
#define CONFIG_H

#include "module.h"

static const char *font =
"JetBrainsMono Nerd Font:size=10";

static const char *bg = "#000000";
static const char *fg = "#ffffff";

static int padding = 20;
static int bar_height = 42;

/* LEFT MODULES */

static Module left_modules[] = {
    { "scripts/workspace.sh", "" }
};

static int left_count =
sizeof(left_modules)/sizeof(Module);

/* CENTER MODULES */

static Module center_modules[] = {
    { "scripts/clock.sh", "" }
};

static int center_count =
sizeof(center_modules)/sizeof(Module);

/* RIGHT MODULES */

static Module right_modules[] = {
    { "scripts/mem.sh", "" },
    { "scripts/cpu.sh", "" }
};

static int right_count =
sizeof(right_modules)/sizeof(Module);

#endif
