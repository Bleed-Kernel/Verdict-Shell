#include <stdio.h>
#include <string.h>
#include <syscalls/yeild.h>
#include <commands/commands.h>
#include "impl/_impls.h"                 // fucking genius- put a _ at the start so it appears first, take notes folks

static builtin_t builtins[] = {
    { "cat", cmd_cat },
    { "clear", cmd_clear },
    { "echo", cmd_echo },
    { "exit", cmd_exit },
    { "kill", cmd_kill },
    { "reboot", cmd_reboot },
    { "shutdown", cmd_shutdown },
    { "spawn", cmd_spawn }, 
    { "cd", cmd_chdir },
    { "ls", cmd_ls },
    { "timeis", cmd_time }
};

int builtin_dispatch(shell_cmd_t *cmd) {
    for (unsigned i = 0; i < sizeof(builtins)/sizeof(builtins[0]); i++) {
        if (!strcmp(cmd->argv[0], builtins[i].name))
            return builtins[i].fn(cmd);
    }
    return -1;
}
