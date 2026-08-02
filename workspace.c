#define _POSIX_C_SOURCE 200809L

#include "workspace.h"

#include <X11/Xatom.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

/* ── i3 / sway IPC (both speak the same wire protocol) ───────────────── */

#define IPC_MAGIC     "i3-ipc"
#define IPC_MAGIC_LEN 6
#define MSG_GET_WORKSPACES 1
#define MSG_SUBSCRIBE      2

static int ipc_socket_connect(const char *path)
{
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

/* Try, in order: $I3SOCK, $SWAYSOCK, `i3 --get-socketpath`,
 * `swaymsg --get-socketpath`. Works for both i3 and sway. */
static int ipc_connect(void)
{
    const char *env;
    static char buf[108];

    if ((env = getenv("I3SOCK")))   return ipc_socket_connect(env);
    if ((env = getenv("SWAYSOCK"))) return ipc_socket_connect(env);

    const char *probes[] = {
        "i3 --get-socketpath 2>/dev/null",
        "swaymsg --get-socketpath 2>/dev/null",
        NULL
    };

    for (int i = 0; probes[i]; i++) {
        FILE *fp = popen(probes[i], "r");
        if (!fp) continue;
        if (fgets(buf, sizeof(buf), fp)) {
            pclose(fp);
            buf[strcspn(buf, "\n")] = 0;
            if (*buf) {
                int fd = ipc_socket_connect(buf);
                if (fd >= 0) return fd;
            }
        } else {
            pclose(fp);
        }
    }
    return -1;
}

static int ipc_send(int fd, uint32_t type, const char *payload)
{
    uint32_t len = payload ? strlen(payload) : 0;
    char hdr[IPC_MAGIC_LEN + 8];
    memcpy(hdr, IPC_MAGIC, IPC_MAGIC_LEN);
    memcpy(hdr + IPC_MAGIC_LEN,     &len,  4);
    memcpy(hdr + IPC_MAGIC_LEN + 4, &type, 4);
    if (write(fd, hdr, sizeof(hdr)) < 0) return -1;
    if (len && write(fd, payload, len) < 0) return -1;
    return 0;
}

static char *ipc_recv(int fd)
{
    char hdr[IPC_MAGIC_LEN + 8];
    if (read(fd, hdr, sizeof(hdr)) != (ssize_t)sizeof(hdr)) return NULL;

    uint32_t len;
    memcpy(&len, hdr + IPC_MAGIC_LEN, 4);

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

static int ipc_parse_workspaces(const char *data, WsItem *out, int max)
{
    int count = 0;
    const char *p = data;

    while (count < max && (p = strstr(p, "\"num\":")))
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

        out[count].num     = num;
        out[count].focused = is_focused;
        count++;
    }
    return count;
}

static int ipc_get_items(WsItem *out, int max)
{
    int fd = ipc_connect();
    if (fd < 0) return 0;
    ipc_send(fd, MSG_GET_WORKSPACES, NULL);
    char *data = ipc_recv(fd);
    close(fd);
    if (!data) return 0;
    int count = ipc_parse_workspaces(data, out, max);
    free(data);
    return count;
}

/* ── EWMH fallback (bspwm / openbox / dwm+ewmh / anything compliant) ── */

static long ewmh_get_cardinal(Display *dpy, Window root, Atom atom, long fallback)
{
    Atom type; int format; unsigned long n, extra;
    unsigned char *data = NULL;
    long val = fallback;

    if (XGetWindowProperty(dpy, root, atom, 0, 1, False, XA_CARDINAL,
                            &type, &format, &n, &extra, &data) == Success && data) {
        if (n >= 1) val = *(long*)data;
        XFree(data);
    }
    return val;
}

static int ewmh_get_items(Display *dpy, Window root, Atom a_cur, Atom a_num,
                           WsItem *out, int max)
{
    long current = ewmh_get_cardinal(dpy, root, a_cur, 0);
    long total   = ewmh_get_cardinal(dpy, root, a_num, 1);
    if (total < 1) total = 1;
    if (total > max) total = max;

    for (long i = 0; i < total; i++) {
        out[i].num     = (int)(i + 1);
        out[i].focused = (i == current);
    }
    return (int)total;
}

/* ── public API ───────────────────────────────────────────────────── */

void ws_init(Workspace *ws, Display *dpy, Window root)
{
    ws->dpy  = dpy;
    ws->root = root;
    ws->a_current_desktop = XInternAtom(dpy, "_NET_CURRENT_DESKTOP",   False);
    ws->a_num_desktops    = XInternAtom(dpy, "_NET_NUMBER_OF_DESKTOPS", False);

    int fd = ipc_connect();
    if (fd >= 0) {
        ipc_send(fd, MSG_SUBSCRIBE, "[\"workspace\"]");
        char *ack = ipc_recv(fd);
        free(ack);
        ws->backend = WS_IPC;
        ws->ipc_fd  = fd;
        return;
    }

    /* EWMH: watch the root window for desktop-switch property changes */
    XSelectInput(dpy, root, PropertyChangeMask);
    ws->backend = WS_EWMH;
    ws->ipc_fd  = -1;
}

int ws_fd(const Workspace *ws)
{
    return (ws->backend == WS_IPC) ? ws->ipc_fd : -1;
}

void ws_drain_ipc(Workspace *ws)
{
    if (ws->backend != WS_IPC) return;
    char *msg = ipc_recv(ws->ipc_fd);
    free(msg);
}

int ws_handle_xevent(Workspace *ws, XEvent *ev)
{
    if (ws->backend != WS_EWMH) return 0;
    if (ev->type != PropertyNotify) return 0;
    if (ev->xproperty.window != ws->root) return 0;
    return ev->xproperty.atom == ws->a_current_desktop;
}

int ws_get_items(Workspace *ws, WsItem *out, int max)
{
    switch (ws->backend) {
        case WS_IPC:  return ipc_get_items(out, max);
        case WS_EWMH: return ewmh_get_items(ws->dpy, ws->root,
                                             ws->a_current_desktop,
                                             ws->a_num_desktops, out, max);
        default:      return 0;
    }
}
