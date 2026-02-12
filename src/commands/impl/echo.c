#include <stdio.h>
#include <commands/commands.h>

int cmd_echo(shell_cmd_t *cmd) {
    (void)cmd;

    if (cmd->argc < 0){
        printf("Bad Command Usage: echo requires an arguemnt to\n"); 
        return 0;
    }

    for (int i = 1; i < cmd->argc; i++){
        printf("%s ", cmd->argv[i]);
    }

    printf("\n");
    return 0;
}