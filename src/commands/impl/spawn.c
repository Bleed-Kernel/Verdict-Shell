#include <ansii.h>
#include <stdio.h>
#include <syscalls/spawn.h>
#include <syscalls/waitpid.h>
#include <syscalls/getcwd.h>
#include <commands/commands.h>
#include <string.h>
#include <syscalls/exit.h>
#include <limits.h>

int cmd_spawn(shell_cmd_t *cmd){
    if (cmd->argc < 2){
        printf("Usage: spawn <program>\n");
        return -1;
    }

    const char *prog = cmd->argv[1];
    char resolved[PATH_MAX];

    if (prog[0] == '/') {
        strncpy(resolved, prog, PATH_MAX - 1);
        resolved[PATH_MAX - 1] = '\0';
    } else {
        char cwd[PATH_MAX] = {0};
        if (!_getcwd(cwd, PATH_MAX)) {
            printf("spawn: cannot get current directory\n");
            return -1;
        }

        size_t cwd_len = strlen(cwd);
        if (cwd_len == 1 && cwd[0] == '/') {
            snprintf(resolved, PATH_MAX, "/%s", prog);
        } else {
            snprintf(resolved, PATH_MAX, "%s/%s", cwd, prog);
        }
    }


    int child_argc = cmd->argc - 1;
    const char *const *child_argv = (const char *const *)&cmd->argv[1];

    int pid = _spawn(resolved, child_argv, (uint64_t)child_argc);

    if (pid < 0){
        printf(LOG_ERROR "Kernel Attempted to start Program but got error code: %d\n", pid);
        return pid;
    }
    return 0;
}