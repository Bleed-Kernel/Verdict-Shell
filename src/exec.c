#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <syscalls/exit.h>
#include <syscalls/mmap.h>
#include <syscalls/munmap.h>
#include <commands/commands.h>
#include <ansii.h>
#include <theme.h>
#include <main.h>
#include <ipc.h>
#include <errno.h>

#define IPC_PAGE_SIZE 4096UL

static int send_pipe_payload(pid_t target_pid, const char *payload) {
    if (!payload || !*payload)
        return -1;

    size_t bytes = strlen(payload) + 1;
    size_t pages = (bytes + (IPC_PAGE_SIZE - 1)) / IPC_PAGE_SIZE;
    if (pages == 0)
        pages = 1;

    char *region = (char *)_mmap(pages);
    if (!region)
        return -1;

    memcpy(region, payload, bytes);

    if (ipc_send(target_pid, region, pages) < 0) {
        printf("ipc-send: failed (errno=%d: %s)\n", errno, strerror(errno));
        _munmap(region);
        return -1;
    }

    return 0;
}

int shell_execute(shell_cmd_t *cmd) {
    if (cmd->argc == 0)
        return 0;

    if (builtin_dispatch(cmd) == 0)
        return 0;

    const char *path = NULL;
    const char *name = cmd->argv[0];

    if (name[0] == '.' && name[1] == '/') {
        name += 2;
    }

    if (strchr(cmd->argv[0], '/')) {
        path = cmd->argv[0];
    } else {
        path = path_resolve(name);
    }

    if (!path) {
        printf("command not found: %s\n", cmd->argv[0]);
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        printf("failed to fork for command: %s\n", cmd->argv[0]);
        return -1;
    }

    if (pid == 0) {
        execv(path, (char *const *)cmd->argv);
        printf("%s" "%s" RESET " is not a valid executable ELF file, so the kernel cannot start it.\n",
               theme_secondary_fg(),
               name);
        _exit(127);
    }

    if (cmd->pipe_in) {
        if (strcmp(cmd->argv[0], "ipc-send") != 0) {
            if (send_pipe_payload(pid, cmd->pipe_in) < 0) {
                printf("pipeline send to pid %d failed\n", (int)pid);
            }
        }
    }

    waitpid(pid, NULL, 0);
    return 0;
}
