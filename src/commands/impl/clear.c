#include <stdio.h>
#include <commands/commands.h>
#include <main.h>

int cmd_clear(shell_cmd_t *cmd) {
    (void)cmd;
    printf("\x1b[H\x1b[2J\x1b[3J");
    return 0;
}