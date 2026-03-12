#include <stdio.h>
#include <string.h>
#include "i3ipc.h"

void i3_get_workspaces(char *buf, int size)
{
    FILE *fp = popen("i3-msg -t get_workspaces", "r");
    if(!fp) {
        snprintf(buf, size, " ?");
        return;
    }

    char data[4096] = {0};
    fread(data, 1, sizeof(data)-1, fp);
    pclose(fp);

    char *focused = strstr(data, "\"focused\":true");

    if(focused)
        snprintf(buf, size, " ws"); // placeholder
    else
        snprintf(buf, size, " ?");
}
