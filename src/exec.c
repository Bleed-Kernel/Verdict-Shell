#include <string.h>
#include <stdio.h>
#include <syscalls/spawn.h>
#include <syscalls/waitpid.h>
#include <commands/commands.h>
#include <ansii.h>
#include <main.h>

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

    if (name[0] == '/') {
        path = name;
    } else {
        path = path_resolve(name);
    }

    if (!path) {
        printf("command not found: %s\n", cmd->argv[0]);
        return -1;
    }

    int pid = _spawn(path, cmd->argv, cmd->argc);
    if (pid < 0) {
        printf(RGB_FG(212, 44, 44) "%s" RESET " is not a valid executable ELF file, so the kernel cannot start it.\n", name);
        return -1;
    }

    _waitpid(pid);
    return 0;
}