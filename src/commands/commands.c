#include <stdio.h>
#include <string.h>
#include <syscalls.h>
#include <commands/commands.h>
#include "impl/_impls.h"                 // fucking genius- put a _ at the start so it appears first, take notes folks

static builtin_t builtins[] = {
    { "clear", cmd_clear },
    { "exit", cmd_exit },
    { "cd", cmd_chdir },
    { "export", cmd_export },
    { "unset", cmd_unset },
    { "env", cmd_env },
    { "true", cmd_true },
    { "false", cmd_false },
};

const builtin_t *builtin_find(const char *name) {
    for (unsigned i = 0; i < sizeof(builtins)/sizeof(builtins[0]); i++) {
        if (!strcmp(name, builtins[i].name))
            return &builtins[i];
    }
    return NULL;
}
