#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

#include <commands/commands.h>

int cmd_kill(shell_cmd_t *cmd) {
    int pid;
    int sig = SIGTERM;

    if (cmd->argc < 2) {
        printf("Usage: kill <pid> [signal]\n");
        return -1;
    }

    pid = atoi(cmd->argv[1]);
    if (cmd->argc >= 3)
        sig = atoi(cmd->argv[2]);

    if (kill(pid, sig) < 0) {
        perror("kill");
        return -1;
    }

    return 0;
}