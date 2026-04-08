#include <ansii.h>
#include <stdio.h>
#include <commands/commands.h>
#include <string.h>
#include <mount.h>
#include <ansii.h>
#include <stdio.h>
#include <commands/commands.h>
#include <string.h>
#include <errno.h>
#include <sys/mount.h>

int cmd_mount(shell_cmd_t *cmd) {
    if (cmd->argc != 3) {
        printf("Usage: mount <device> <mountpoint>\n");
        printf("  e.g. mount /dev/hda1 /mnt\n");
        return -1;
    }

    const char *source = cmd->argv[1];
    const char *target = cmd->argv[2];

    int r = _mount(source, target, NULL, 0, NULL);
    if (r < 0) {
        printf(LOG_ERROR "mount: %s -> %s: %s (errno %d)\n",
               source, target, strerror(errno), errno);
        return -1;
    }

    printf("mounted %s at %s\n", source, target);
    return 0;
}

int cmd_umount(shell_cmd_t *cmd) {
    if (cmd->argc != 2) {
        printf("Usage: umount <mountpoint>\n");
        return -1;
    }

    const char *target = cmd->argv[1];

    int r = _umount(target);
    if (r < 0) {
        printf(LOG_ERROR "umount: %s: %s (errno %d)\n",
               target, strerror(errno), errno);
        return -1;
    }

    printf("unmounted %s\n", target);
    return 0;
}