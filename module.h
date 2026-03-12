#ifndef MODULE_H
#define MODULE_H

typedef enum {
    MOD_SCRIPT,   /* legacy popen script */
    MOD_CLOCK,
    MOD_CPU,
    MOD_MEM,
    MOD_WORKSPACE /* handled separately in bar.c via i3ipc */
} ModuleType;

typedef struct {
    ModuleType   type;
    const char  *script;   /* only used for MOD_SCRIPT */
    char         output[256];
} Module;

void module_run(Module *m);

#endif
