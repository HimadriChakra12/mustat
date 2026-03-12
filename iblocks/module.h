#ifndef MODULE_H
#define MODULE_H

typedef enum { MOD_SCRIPT, MOD_CLOCK, MOD_CPU, MOD_MEM } ModuleType;

typedef struct {
    ModuleType   type;
    const char  *script;
    const char  *pos;     /* "LEFT", "CENTER", "RIGHT" */
    int          slot;
    int          interval; /* seconds */
    char         output[256];
} Block;

void block_run(Block *b);

#endif
