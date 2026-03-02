#include <ansii.h>
#include <stdio.h>
#include <commands/commands.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/wait.h>
#include <syscalls/exit.h>

int cmd_spawn(shell_cmd_t *cmd){
    if (cmd->argc < 2){
        printf("Usage: spawn <program> [args ...]\n");
        return -1;
    }

    const char *prog = cmd->argv[1];
    const char *resolved = NULL;

    if (strchr(prog, '/')) {
        resolved = prog;
    } else {
        resolved = path_resolve(prog);
    }

    if (!resolved) {
        printf("spawn: program not found: %s\n", prog);
        return -1;
    }

    char *const *child_argv = (char *const *)&cmd->argv[1];

    pid_t pid = fork();
    if (pid < 0) {
        printf(LOG_ERROR "spawn: failed to fork\n");
        return -1;
    }

    if (pid == 0) {
        execv(resolved, child_argv);
        printf(LOG_ERROR "spawn: exec failed: %s\n", prog);
        _exit(127);
    }

    waitpid(pid, NULL, 0);
    return 0;
}
