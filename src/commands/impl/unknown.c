#include <commands/commands.h>
#include <string.h>
#include <ansii.h>
#include <stdio.h>

int cmd_unknown(shell_cmd_t *cmd) {
    (void)cmd;
    printf("%s%s:%s Unknown Command\n", cmd, LIGHT_GRAY_FG, RESET);
    return 0;
}