#define _POSIX_C_SOURCE 200809L

#include "i3ipc.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define I3_MAGIC     "i3-ipc"
#define I3_MAGIC_LEN 6
#define MSG_GET_WORKSPACES 1
#define MSG_SUBSCRIBE      2

static int i3_connect(void)
{
    const char *path = getenv("I3SOCK");
    static char buf[108];
    if (!path) {
        FILE *fp = popen("i3 --get-socketpath 2>/dev/null", "r");
        if (!fp) return -1;
        if (!fgets(buf, sizeof(buf), fp)) { pclose(fp); return -1; }
        pclose(fp);
        buf[strcspn(buf, "\n")] = 0;
        path = buf;
    }

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s", path);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static int i3_send(int fd, uint32_t type, const char *payload)
{
    uint32_t len = payload ? strlen(payload) : 0;
    char hdr[I3_MAGIC_LEN + 8];
    memcpy(hdr, I3_MAGIC, I3_MAGIC_LEN);
    memcpy(hdr + I3_MAGIC_LEN,     &len,  4);
    memcpy(hdr + I3_MAGIC_LEN + 4, &type, 4);
    if (write(fd, hdr, sizeof(hdr)) < 0) return -1;
    if (len && write(fd, payload, len) < 0) return -1;
    return 0;
}

static char *i3_recv(int fd)
{
    char hdr[I3_MAGIC_LEN + 8];
    if (read(fd, hdr, sizeof(hdr)) != (ssize_t)sizeof(hdr)) return NULL;

    uint32_t len;
    memcpy(&len, hdr + I3_MAGIC_LEN, 4);

    char *buf = malloc(len + 1);
    if (!buf) return NULL;

    uint32_t got = 0;
    while (got < len) {
        int n = read(fd, buf + got, len - got);
        if (n <= 0) { free(buf); return NULL; }
        got += n;
    }
    buf[len] = 0;
    return buf;
}

static void parse_workspaces(const char *data, char *buf, int size)
{
    int nums[16], focused[16], count = 0;
    const char *p = data;

    while (count < 16 && (p = strstr(p, "\"num\":")))
    {
        p += 6;
        while (*p == ' ') p++;
        int num = atoi(p);

        const char *f = strstr(p, "\"focused\":");
        int is_focused = 0;
        if (f) {
            f += 10;
            while (*f == ' ') f++;
            is_focused = (strncmp(f, "true", 4) == 0);
        }
        nums[count]    = num;
        focused[count] = is_focused;
        count++;
    }

    if (count == 0) { snprintf(buf, size, " ?"); return; }

    int off = 0;
    for (int i = 0; i < count && off < size - 8; i++)
        off += snprintf(buf + off, size - off,
                        focused[i] ? "[%d] " : "%d ", nums[i]);

    if (off > 0 && buf[off-1] == ' ') buf[off-1] = 0;
}

/* Open a persistent socket subscribed to workspace events */
int i3_subscribe_workspaces(void)
{
    int fd = i3_connect();
    if (fd < 0) return -1;
    i3_send(fd, MSG_SUBSCRIBE, "[\"workspace\"]");
    /* drain the subscribe ack */
    char *ack = i3_recv(fd);
    free(ack);
    return fd;
}

/* Call after select() says fd is readable to discard the event payload */
void i3_drain_event(int fd)
{
    char *msg = i3_recv(fd);
    free(msg);
}

/* Open a fresh connection, query workspaces, close */
void i3_get_workspaces(int fd_unused, char *buf, int size)
{
    (void)fd_unused;
    int fd = i3_connect();
    if (fd < 0) { snprintf(buf, size, " ?"); return; }
    i3_send(fd, MSG_GET_WORKSPACES, NULL);
    char *data = i3_recv(fd);
    close(fd);
    if (!data) { snprintf(buf, size, " ?"); return; }
    parse_workspaces(data, buf, size);
    free(data);
}
