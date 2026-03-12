#ifndef I3IPC_H
#define I3IPC_H

int  i3_subscribe_workspaces(void);
void i3_drain_event(int fd);
void i3_get_workspaces(int fd, char *buf, int size);

#endif
