#include <commands/commands.h>
#include <ipc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <syscalls/mmap.h>
#include <syscalls/munmap.h>

#define IPC_PAGE_SIZE 4096UL

static int parse_target_pid(const char *s, pid_t *out) {
    if (!s || !*s || !out)
        return -1;

    char *end = NULL;
    long v = strtol(s, &end, 10);
    if (!end || *end != '\0' || v <= 0)
        return -1;

    *out = (pid_t)v;
    return 0;
}

static int join_payload(shell_cmd_t *cmd, char *dst, size_t dst_size) {
    size_t off = 0;

    if (cmd->pipe_in && cmd->argc == 2) {
        size_t n = strlen(cmd->pipe_in);
        if (n + 1 > dst_size)
            return -1;
        memcpy(dst, cmd->pipe_in, n + 1);
        return 0;
    }

    if (cmd->argc < 3)
        return -1;

    for (int i = 2; i < cmd->argc; i++) {
        const char *part = cmd->argv[i];
        size_t n = strlen(part);
        if (off + n + 2 > dst_size)
            return -1;

        memcpy(dst + off, part, n);
        off += n;
        if (i + 1 < cmd->argc)
            dst[off++] = ' ';
    }

    dst[off] = '\0';
    return 0;
}

int cmd_ipc_send(shell_cmd_t *cmd) {
    if (!cmd || cmd->argc < 2) {
        printf("usage: ipc-send <pid> [message...]\n");
        printf("       <text> > ipc-send <pid>\n");
        return -1;
    }

    pid_t target = 0;
    if (parse_target_pid(cmd->argv[1], &target) != 0) {
        printf("ipc-send: invalid pid: %s\n", cmd->argv[1]);
        return -1;
    }

    char local_payload[IPC_PAGE_SIZE];
    if (join_payload(cmd, local_payload, sizeof(local_payload)) != 0) {
        printf("ipc-send: missing/oversized payload\n");
        return -1;
    }

    size_t bytes = strlen(local_payload) + 1;
    size_t pages = (bytes + (IPC_PAGE_SIZE - 1)) / IPC_PAGE_SIZE;
    if (pages == 0)
        pages = 1;

    char *region = (char *)_mmap(pages);
    if (!region) {
        printf("ipc-send: mmap failed\n");
        return -1;
    }

    memcpy(region, local_payload, bytes);

    if (ipc_send(target, region, pages) < 0) {
        printf("ipc-send: failed (errno=%d: %s)\n", errno, strerror(errno));
        _munmap(region);
        return -1;
    }

    return 0;
}
