#ifndef MODULE_H
#define MODULE_H

typedef struct {
    const char *name;
    const char *script;
    int interval;   // refresh interval in seconds
    int counter;    // internal counter
    char output[256];
} Module;

void module_update(Module *m);

#endif
