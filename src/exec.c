#include <string.h>
#include <stdlib.h>
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
        printf("pipeline IPC send failed (errno=%d: %s)\n", errno, strerror(errno));
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

static char *run_substitution(const char *inner_cmd) {
    if (!inner_cmd || !*inner_cmd)
        return NULL;

    int fds[2];
    if (pipe(fds) < 0)
        return NULL;

    pid_t pid = fork();
    if (pid < 0) {
        close(fds[0]);
        close(fds[1]);
        return NULL;
    }

    if (pid == 0) {
        if (dup2(fds[1], STDOUT_FILENO) < 0)
            _exit(126);
        close(fds[0]);
        close(fds[1]);

        char inner_copy[SHELL_MAX_LINE];
        strncpy(inner_copy, inner_cmd, SHELL_MAX_LINE - 1);
        inner_copy[SHELL_MAX_LINE - 1] = '\0';

        shell_cmd_t sub;
        if (shell_parse(inner_copy, &sub) < 0)
            _exit(1);

        if (builtin_dispatch(&sub) == 0)
            _exit(0);

        const char *path = resolve_exec_path(sub.argv[0]);
        if (!path)
            _exit(127);

        execv(path, (char *const *)sub.argv);
        _exit(127);
    }

    close(fds[1]);

    size_t cap = 4096;
    size_t total = 0;
    int truncated = 0;
    char *out = malloc(cap);

    while (out) {
        if (total + 1 == cap) {
            if (cap >= SHELL_MAX_SUBST_LEN) {
                truncated = 1;
                break;
            }
            size_t new_cap = cap * 2 > SHELL_MAX_SUBST_LEN ? SHELL_MAX_SUBST_LEN : cap * 2;
            char *grown = realloc(out, new_cap);
            if (!grown) {
                truncated = 1;
                break;
            }
            out = grown;
            cap = new_cap;
        }

        ssize_t n = read(fds[0], out + total, cap - 1 - total);
        if (n <= 0)
            break;
        total += (size_t)n;
    }
    close(fds[0]);
    waitpid(pid, NULL, 0);

    if (!out)
        return NULL;

    if (truncated)
        printf(LOG_WARN "$(%s) output truncated to %zu bytes\n", inner_cmd, total);

    while (total > 0 && (out[total - 1] == '\n' || out[total - 1] == '\r'))
        total--;
    out[total] = '\0';

    return out;
}

static int resolve_substitutions(shell_cmd_t *cmd) {
    for (int i = 0; i < cmd->argc; i++) {
        int sidx = cmd->subst_is_subst[i];
        if (!sidx)
            continue;

        sidx--;
        cmd->subst_buf[sidx] = run_substitution(cmd->subst_inner[sidx]);
        cmd->argv[i] = cmd->subst_buf[sidx] ? cmd->subst_buf[sidx] : "";
    }

    return 0;
}

static void free_substitutions(shell_cmd_t *cmd) {
    for (int i = 0; i < SHELL_MAX_SUBST; i++) {
        free(cmd->subst_buf[i]);
        cmd->subst_buf[i] = NULL;
    }
}

static int execute_process_pipe(shell_cmd_t *cmd) {
    const char **producer_argv = cmd->reverse_process_pipe ? cmd->pipe_argv : cmd->argv;
    int producer_argc = cmd->reverse_process_pipe ? cmd->pipe_argc : cmd->argc;
    const char **consumer_argv = cmd->reverse_process_pipe ? cmd->argv : cmd->pipe_argv;
    int consumer_argc = cmd->reverse_process_pipe ? cmd->argc : cmd->pipe_argc;

    if (producer_argc == 0 || consumer_argc == 0)
        return -1;

    const char *resolved = resolve_exec_path(producer_argv[0]);
    if (!resolved) {
        printf("command not found: %s\n", producer_argv[0]);
        return -1;
    }
    char producer_path[SHELL_MAX_PATH_LEN * 2];
    strncpy(producer_path, resolved, sizeof(producer_path) - 1);
    producer_path[sizeof(producer_path) - 1] = '\0';

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
        if (dup2(fds[1], STDOUT_FILENO) < 0) {
            printf("pipeline: dup2 stdout failed (errno=%d: %s)\n", errno, strerror(errno));
            _exit(126);
        }
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
        if (dup2(fds[0], STDIN_FILENO) < 0) {
            printf("pipeline: dup2 stdin failed (errno=%d: %s)\n", errno, strerror(errno));
            _exit(126);
        }
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

static int execute_resolved(shell_cmd_t *cmd);

int shell_execute(shell_cmd_t *cmd) {
    if (cmd->argc == 0)
        return 0;

    resolve_substitutions(cmd);
    int rc = execute_resolved(cmd);
    free_substitutions(cmd);
    return rc;
}

static int execute_resolved(shell_cmd_t *cmd) {
    if (cmd->has_process_pipe)
        return execute_process_pipe(cmd);

    if (builtin_dispatch(cmd) == 0)
        return 0;

    const char *path = resolve_exec_path(cmd->argv[0]);
    const char *name = cmd->argv[0];

    if (strcmp(name, "write") == 0 && has_been_warnned == 0){
        printf(RED_FG "WARNING!%s 'write' is a particularly dangerous program when used on Physical Disks and can cause perminant data loss\n", WHITE_FG);
        printf("It is reccomended that you never use write on a physical disk unless you know what you are doing.\n");
        printf(YELLOW_FG "Please review the command you have written and re-enter it or hit up arrow to send it again if it is safe, you have been warned\n");
        has_been_warnned = 1;
        return 0;
    }

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
        if (send_pipe_payload(pid, cmd->pipe_in) < 0) {
            printf("pipeline send to pid %d failed\n", (int)pid);
        }
    }

    waitpid(pid, NULL, 0);
    return 0;
}