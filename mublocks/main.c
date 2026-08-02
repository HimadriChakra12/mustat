#define _POSIX_C_SOURCE 200809L

#include "module.h"
#include "lua_config.h"
#include "../ipc.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/un.h>

static int mustat_connect(void)
{
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", MUSTAT_SOCKET);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

int main(void)
{
    Block *blocks = NULL;
    int count = lua_config_load(&blocks);

    if (count == 0) {
        fprintf(stderr,
            "mustat-blocks: no mu.lua found (or it registered nothing).\n"
            "Looked in $XDG_CONFIG_HOME/mustat/, ~/.config/mustat/, /etc/mustat/.\n");
        return 1;
    }

    time_t *last = calloc(count, sizeof(time_t));

    int fd = -1;
    while (fd < 0) {
        fd = mustat_connect();
        if (fd < 0) sleep(1);
    }

    while (1) {
        time_t now = time(NULL);
        int dirty = 0;

        for (int i = 0; i < count; i++) {
            if (now - last[i] < blocks[i].interval) continue;

            block_run(&blocks[i]);
            last[i] = now;

            char line[768];
            snprintf(line, sizeof(line), "%s:%s:%s\n",
                     blocks[i].pos, blocks[i].name, blocks[i].output);

            if (write(fd, line, strlen(line)) < 0) {
                close(fd);
                fd = -1;
                while (fd < 0) { fd = mustat_connect(); sleep(1); }
            }
            dirty = 1;
        }

        if (!dirty) {
            struct timespec ts = { .tv_sec = 0, .tv_nsec = 100000000 };
            nanosleep(&ts, NULL);
        }
    }
}
