#ifndef CONFIG_H
#define CONFIG_H

#include "module.h"

// left, center, right modules
static Module left_modules[] = {
    { "cpu", "scripts/cpu.sh", 2 },
};

static Module center_modules[] = {
    { "time", "scripts/time.sh", 1 },
};

static Module right_modules[] = {
    { "mem", "scripts/mem.sh", 2 },
};

#endif
