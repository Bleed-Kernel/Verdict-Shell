#include <commands/commands.h>
#include <ipc.h>
#include <stdio.h>
#include <string.h>
#include <syscalls/munmap.h>
#include <unistd.h>
#include <errno.h>

#define IPC_PAGE_SIZE 4096UL

static size_t bounded_strlen(const char *s, size_t max) {
    size_t i = 0;
    while (i < max && s[i] != '\0')
        i++;
    return i;
}

int cmd_ipc_recv(shell_cmd_t *cmd) {
    (void)cmd;

    ipc_message_t msg;
    if (ipc_recv(&msg) < 0) {
        if (errno == EAGAIN) {
            printf("ipc-recv: no pending IPC messages\n");
            return 0;
        }

        printf("ipc-recv: failed (errno=%d: %s)\n", errno, strerror(errno));
        return -1;
    }

    char *payload = (char *)(uintptr_t)msg.addr;
    size_t max_bytes = (size_t)msg.pages * IPC_PAGE_SIZE;
    size_t n = bounded_strlen(payload, max_bytes);

    if (n > 0)
        (void)write(1, payload, n);
    (void)write(1, "\n", 1);

    _munmap((void *)(uintptr_t)msg.addr);
    return 0;
}
