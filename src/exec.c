#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <syscalls.h>
#include <commands/commands.h>
#include <ansii.h>
#include <theme.h>
#include <main.h>
#include <ipc.h>
#include <errno.h>

#define IPC_PAGE_SIZE 4096UL

int pipe(int fds[2]);
int dup2(int oldfd, int newfd);

int shell_last_status = 0;
static int has_been_warnned = 0;

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

    return path_resolve(name);
}

static int wait_status(pid_t pid) {
    int st = 0;

    while (waitpid(pid, &st, 0) < 0) {
        if (errno != EINTR)
            return 1;
    }

    return WIFSIGNALED(st) ? 128 + WTERMSIG(st) : WEXITSTATUS(st);
}

static int is_assignment(const char *s) {
    if (!((*s >= 'a' && *s <= 'z') || (*s >= 'A' && *s <= 'Z') || *s == '_'))
        return 0;
    while ((*s >= 'a' && *s <= 'z') || (*s >= 'A' && *s <= 'Z') || (*s >= '0' && *s <= '9') || *s == '_')
        s++;
    return *s == '=';
}

static void take_assignments(shell_cmd_t *cmd) {
    int n = 0;

    while (n < cmd->argc && is_assignment(cmd->argv[n])) {
        char name[128];
        const char *eq = strchr(cmd->argv[n], '=');
        size_t len = (size_t)(eq - cmd->argv[n]);
        if (len >= sizeof(name))
            len = sizeof(name) - 1;
        memcpy(name, cmd->argv[n], len);
        name[len] = '\0';
        setenv(name, eq + 1, 1);
        n++;
    }

    if (!n)
        return;
    memmove(cmd->argv, cmd->argv + n, (size_t)(cmd->argc - n + 1) * sizeof(cmd->argv[0]));
    cmd->argc -= n;
}

static int has_redirects(const shell_cmd_t *cmd) {
    return cmd->in_path || cmd->out_path || cmd->err_path || cmd->err_to_out;
}

static int redirect(const char *path, int flags, int target) {
    int fd = open(path, flags);
    if (fd < 0) {
        fprintf(stderr, "verdict: cant open %s: %s\n", path, strerror(errno));
        return -1;
    }

    if (fd != target) {
        dup2(fd, target);
        close(fd);
    }
    return 0;
}

static int apply_redirects(const shell_cmd_t *cmd) {
    if (cmd->in_path && redirect(cmd->in_path, O_RDONLY, STDIN_FILENO) < 0)
        return -1;
    if (cmd->out_path && redirect(cmd->out_path, O_WRONLY | O_CREAT | (cmd->out_append ? O_APPEND : O_TRUNC), STDOUT_FILENO) < 0)
        return -1;
    if (cmd->err_to_out)
        dup2(STDOUT_FILENO, STDERR_FILENO);
    if (cmd->err_path && redirect(cmd->err_path, O_WRONLY | O_CREAT | (cmd->err_append ? O_APPEND : O_TRUNC), STDERR_FILENO) < 0)
        return -1;
    return 0;
}

static void child_exit(int code) {
    fflush(stdout);
    fflush(stderr);
    _exit(code);
}

// runs in the forked child with the pipe ends already in place, never comes back
static void run_child(shell_cmd_t *cmd) {
    if (apply_redirects(cmd) < 0)
        child_exit(1);

    take_assignments(cmd);
    if (!cmd->argc)
        child_exit(0);

    const builtin_t *builtin = builtin_find(cmd->argv[0]);
    if (builtin)
        child_exit(builtin->fn(cmd) < 0 ? 1 : 0);

    const char *path = resolve_exec_path(cmd->argv[0]);
    if (!path) {
        fprintf(stderr, "command not found: %s\n", cmd->argv[0]);
        child_exit(127);
    }

    execv(path, (char *const *)cmd->argv);
    fprintf(stderr, "%s" "%s" RESET " is not a valid executable ELF file, so the kernel cannot start it.\n",
            theme_secondary_fg(), cmd->argv[0]);
    child_exit(126);
}

static int run_pipeline(shell_pipeline_t *pl) {
    shell_cmd_t *first = &pl->cmds[0];

    if (pl->count == 1 && !pl->pipe_in && !has_redirects(first)) {
        int all = 1;
        for (int i = 0; i < first->argc; i++)
            all &= is_assignment(first->argv[i]);
        if (all) {
            take_assignments(first);
            return 0;
        }

        const builtin_t *builtin = builtin_find(first->argv[0]);
        if (builtin)
            return builtin->fn(first) < 0 ? 1 : 0;
    }

    if (pl->count == 1 && strcmp(first->argv[0], "write") == 0 && has_been_warnned == 0) {
        printf(RED_FG "WARNING!%s 'write' is a particularly dangerous program when used on Physical Disks and can cause perminant data loss\n", WHITE_FG);
        printf("It is reccomended that you never use write on a physical disk unless you know what you are doing.\n");
        printf(YELLOW_FG "Please review the command you have written and re-enter it or hit up arrow to send it again if it is safe, you have been warned\n");
        has_been_warnned = 1;
        return 0;
    }

    pid_t pids[SHELL_MAX_STAGES];
    int started = 0;
    int in_fd = -1;

    fflush(stdout);
    fflush(stderr);

    for (int i = 0; i < pl->count; i++) {
        int fds[2] = { -1, -1 };
        if (i < pl->count - 1 && pipe(fds) < 0) {
            printf("pipe failed (errno=%d: %s)\n", errno, strerror(errno));
            break;
        }

        pid_t pid = fork();
        if (pid == 0) {
            if (in_fd >= 0) {
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }
            if (fds[1] >= 0) {
                dup2(fds[1], STDOUT_FILENO);
                close(fds[1]);
                close(fds[0]);
            }
            run_child(&pl->cmds[i]);
        }

        if (in_fd >= 0)
            close(in_fd);
        if (fds[1] >= 0)
            close(fds[1]);
        in_fd = fds[0];

        if (pid < 0) {
            printf("failed to fork for command: %s\n", pl->cmds[i].argv[0]);
            break;
        }
        pids[started++] = pid;
    }

    if (in_fd >= 0)
        close(in_fd);

    if (pl->pipe_in && started == 1 && send_pipe_payload(pids[0], pl->pipe_in) < 0)
        printf("pipeline send to pid %d failed\n", (int)pids[0]);

    int status = started == pl->count ? 0 : 1;
    for (int i = 0; i < started; i++) {
        int st = wait_status(pids[i]);
        if (i == pl->count - 1)
            status = st;
    }

    return status;
}

int shell_run_line(const char *line) {
    const char *p = line;
    int run = 1;

    while (*p) {
        shell_pipeline_t pl;
        shell_sep_t sep;

        if (shell_parse(&p, &pl, &sep, !run) < 0) {
            shell_last_status = 2;
            return shell_last_status;
        }

        if (run && pl.count)
            shell_last_status = run_pipeline(&pl);
        shell_pipeline_free(&pl);

        if (sep == SEP_END)
            break;
        if (sep == SEP_SEMI)
            run = 1;
        else if (sep == SEP_AND)
            run = shell_last_status == 0;
        else
            run = shell_last_status != 0;
    }

    return shell_last_status;
}

char *shell_substitute(const char *inner) {
    int fds[2];
    if (pipe(fds) < 0)
        return NULL;

    fflush(stdout);
    pid_t pid = fork();
    if (pid < 0) {
        close(fds[0]);
        close(fds[1]);
        return NULL;
    }

    if (pid == 0) {
        dup2(fds[1], STDOUT_FILENO);
        close(fds[0]);
        close(fds[1]);
        child_exit(shell_run_line(inner));
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
    shell_last_status = wait_status(pid);

    if (!out)
        return NULL;

    if (truncated)
        printf(LOG_WARN "$(%s) output truncated to %zu bytes\n", inner, total);

    while (total > 0 && (out[total - 1] == '\n' || out[total - 1] == '\r'))
        total--;
    out[total] = '\0';

    return out;
}
