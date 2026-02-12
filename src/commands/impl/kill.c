#include <syscalls/tkill.h>
#include <stdlib.h>
#include <commands/commands.h>

int cmd_kill(shell_cmd_t *cmd){
    _tkill(atoi(cmd->argv[1]));
    return 0;
}