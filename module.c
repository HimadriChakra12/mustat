#define _POSIX_C_SOURCE 200809L

#include "module.h"
#include <stdio.h>
#include <string.h>

void module_run(Module *m)
{
    FILE *fp = popen(m->script,"r");

    if(!fp)
        return;

    fgets(m->output,sizeof(m->output),fp);

    m->output[strcspn(m->output,"\n")] = 0;

    pclose(fp);
}
