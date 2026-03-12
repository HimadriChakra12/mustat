#define _POSIX_C_SOURCE 200809L

#include "module.h"
#include "config.h"
#include "../ipc.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/un.h>

static int ibar_connect(void)
{
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", IBAR_SOCKET);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

int main(void)
{
    int fd = -1;

    while (fd < 0) {
        fd = ibar_connect();
        if (fd < 0) sleep(1);
    }

    time_t last[64] = {0};

    while (1) {
        time_t now = time(NULL);
        int dirty = 0;

        for (int i = 0; i < cfg_count; i++) {
            if (now - last[i] >= cfg_blocks[i].interval) {
                block_run(&cfg_blocks[i]);
                last[i] = now;

                char line[512];
                snprintf(line, sizeof(line), "%s:%d:%s\n",
                         cfg_blocks[i].pos,
                         cfg_blocks[i].slot,
                         cfg_blocks[i].output);

                if (write(fd, line, strlen(line)) < 0) {
                    close(fd);
                    fd = -1;
                    while (fd < 0) { fd = ibar_connect(); sleep(1); }
                }
                dirty = 1;
            }
        }

        if (!dirty) {
            struct timespec ts = { .tv_sec = 0, .tv_nsec = 100000000 };
            nanosleep(&ts, NULL);
        }
    }
}
