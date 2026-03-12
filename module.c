#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include "module.h"

void module_update(Module *m)
{
    if(m->counter-- > 0)
        return;
    m->counter = m->interval;

    char cmd[512];
    snprintf(cmd, sizeof(cmd), "bash %s", m->script);

    FILE *fp = popen(cmd, "r");
    if(!fp) return;

    if(fgets(m->output, sizeof(m->output), fp))
        m->output[strcspn(m->output,"\n")] = 0;

    pclose(fp);
}
