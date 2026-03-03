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

int pipe(int fds[2]);
int dup2(int oldfd, int newfd);

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

static const char *resolve_exec_path(const char *name) {
    if (!name || !*name)
        return NULL;

    if (strchr(name, '/'))
        return name;

    const char *trimmed = name;
    if (trimmed[0] == '.' && trimmed[1] == '/')
        trimmed += 2;

    return path_resolve(trimmed);
}

static pid_t spawn_external(const char *path, const char *name, const char **argv) {
    pid_t pid = fork();
    if (pid < 0)
        return -1;

    if (pid == 0) {
        execv(path, (char *const *)argv);
        printf("%s" "%s" RESET " is not a valid executable ELF file, so the kernel cannot start it.\n",
               theme_secondary_fg(),
               name);
        _exit(127);
    }

    return pid;
}

static int execute_process_pipe(shell_cmd_t *cmd) {
    const char **producer_argv = cmd->reverse_process_pipe ? cmd->pipe_argv : cmd->argv;
    int producer_argc = cmd->reverse_process_pipe ? cmd->pipe_argc : cmd->argc;
    const char **consumer_argv = cmd->reverse_process_pipe ? cmd->argv : cmd->pipe_argv;
    int consumer_argc = cmd->reverse_process_pipe ? cmd->argc : cmd->pipe_argc;

    if (producer_argc == 0 || consumer_argc == 0)
        return -1;

    const char *producer_path = resolve_exec_path(producer_argv[0]);
    if (!producer_path) {
        printf("command not found: %s\n", producer_argv[0]);
        return -1;
    }

    const char *consumer_path = resolve_exec_path(consumer_argv[0]);
    if (!consumer_path) {
        printf("command not found: %s\n", consumer_argv[0]);
        return -1;
    }

    int fds[2];
    if (pipe(fds) < 0) {
        printf("pipe failed (errno=%d: %s)\n", errno, strerror(errno));
        return -1;
    }

    pid_t producer_pid = fork();
    if (producer_pid < 0) {
        close(fds[0]);
        close(fds[1]);
        return -1;
    }
    if (producer_pid == 0) {
        dup2(fds[1], STDOUT_FILENO);
        close(fds[0]);
        close(fds[1]);
        execv(producer_path, (char *const *)producer_argv);
        printf("%s" "%s" RESET " failed to execute.\n", theme_secondary_fg(), producer_argv[0]);
        _exit(127);
    }

    pid_t consumer_pid = fork();
    if (consumer_pid < 0) {
        close(fds[0]);
        close(fds[1]);
        waitpid(producer_pid, NULL, 0);
        return -1;
    }
    if (consumer_pid == 0) {
        dup2(fds[0], STDIN_FILENO);
        close(fds[1]);
        close(fds[0]);
        execv(consumer_path, (char *const *)consumer_argv);
        printf("%s" "%s" RESET " failed to execute.\n", theme_secondary_fg(), consumer_argv[0]);
        _exit(127);
    }

    close(fds[0]);
    close(fds[1]);
    waitpid(producer_pid, NULL, 0);
    waitpid(consumer_pid, NULL, 0);
    return 0;
}

int shell_execute(shell_cmd_t *cmd) {
    if (cmd->argc == 0)
        return 0;

    if (cmd->has_process_pipe)
        return execute_process_pipe(cmd);

    if (builtin_dispatch(cmd) == 0)
        return 0;

    const char *path = resolve_exec_path(cmd->argv[0]);
    const char *name = cmd->argv[0];

    if (!path) {
        printf("command not found: %s\n", cmd->argv[0]);
        return -1;
    }

    pid_t pid = spawn_external(path, name, cmd->argv);
    if (pid < 0) {
        printf("failed to fork for command: %s\n", cmd->argv[0]);
        return -1;
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
