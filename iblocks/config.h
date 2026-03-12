#ifndef CONFIG_H
#define CONFIG_H
#include "module.h"
#include <stddef.h>  /* NULL */

static Block cfg_blocks[] = {
    { MOD_CLOCK, NULL, "CENTER", 1, 1, "" },
    { MOD_MEM,   NULL, "RIGHT",  2, 1, "" },
    { MOD_CPU,   NULL, "RIGHT",  2, 2, "" },
};
static int cfg_count = sizeof(cfg_blocks) / sizeof(Block);

#endif
