#include <syscalls/time.h>
#include <time.h>
#include <commands/commands.h>

int cmd_time(shell_cmd_t *cmd){
    (void)cmd;
    time_t time;
    _time(&time);
    print_time(time);
    return 0;
}