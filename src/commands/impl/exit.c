#include <syscalls.h>
#include <commands/commands.h>

int cmd_exit(shell_cmd_t *cmd){
    (void)cmd;
    
    _exit(0);
    return 0;
}