#include <stdio.h>
#include <string.h>
#include <syscalls/yeild.h>
#include <commands/commands.h>
#include "impl/_impls.h"                 // fucking genius- put a _ at the start so it appears first, take notes folks

static builtin_t builtins[] = {
    { "clear", cmd_clear },
    { "exit", cmd_exit },
    { "cd", cmd_chdir },
    { "ls", cmd_ls },
};

int builtin_dispatch(shell_cmd_t *cmd) {
    for (unsigned i = 0; i < sizeof(builtins)/sizeof(builtins[0]); i++) {
        if (!strcmp(cmd->argv[0], builtins[i].name))
            return builtins[i].fn(cmd);
    }
    return -1;
}
