#include <stdio.h>
#include <commands/commands.h>
#include <main.h>

int cmd_clear(shell_cmd_t *cmd) {
    (void)cmd;
    printf("\x1b[J");
    return 0;
}