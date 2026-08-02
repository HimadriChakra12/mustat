#ifndef MODULE_H
#define MODULE_H

typedef enum { MOD_SCRIPT, MOD_CLOCK, MOD_CPU, MOD_MEM } ModuleType;

typedef struct {
    ModuleType   type;
    char         name[64];    /* unique id — this is the key, not slot     */
    char         cmd[512];    /* shell command, only used for MOD_SCRIPT   */
    char         pos[8];      /* "LEFT" / "CENTER" / "RIGHT"                */
    int          interval;    /* seconds                                    */
    int          enabled;
    char         output[256];
} Block;

void block_run(Block *b);

#endif
