#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <commands/commands.h>
#include <libc/mount.h>

int cmd_mount(shell_cmd_t *cmd) {
    if (cmd->argc < 4) {
        printf("usage: mount <source> <target> <fstype> [flags]\n");
        return -1;
    }

    const char *source = cmd->argv[1];
    const char *target = cmd->argv[2];
    const char *fstype = cmd->argv[3];

    unsigned long flags = 0;

    /* we dont have stroul for now but ill do it later and flags will be set up
    if (cmd->argc >= 5) {
        flags = strtoul(cmd->argv[4], NULL, 0);
    }
    */

    const void *data = NULL;

    int ret = _mount(source, target, fstype, flags, data);

    if (ret < 0) {
        printf("mount failed: %d\n", ret);
        return ret;
    }

    return 0;
}