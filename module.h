#ifndef MODULE_H
#define MODULE_H

typedef struct {
    const char *script;
    char output[256];
} Module;

void module_run(Module *m);

#endif
