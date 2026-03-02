#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <syscalls/exit.h>
#include <commands/commands.h>
#include <ansii.h>
#include <theme.h>
#include <main.h>

int shell_execute(shell_cmd_t *cmd) {
    if (cmd->argc == 0)
        return 0;

    if (cmd->pipe_in && strcmp(cmd->argv[0], "ipc-send") != 0) {
        printf("pipeline supports: <text> | ipc-send <pid>\n");
        return -1;
    }

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

    waitpid(pid, NULL, 0);
    return 0;
}
