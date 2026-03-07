#include <stdio.h>

#include <commands/commands.h>
#include <theme.h>
#include <devices/console.h>
#include <syscalls/ioctl.h>

int cmd_theme(shell_cmd_t *cmd) {
    if (cmd->argc < 2) {
        printf("active theme: %s\n", theme_active_path());
        printf("usage: theme <path>\n");
        return 0;
    }

    const char *path = cmd->argv[1];
    if (theme_load(path) < 0) {
        printf("theme: failed to load %s\n", path);
        return -1;
    }

    tty_cursor_t home = { .x = 0, .y = 0 };
    (void)_ioctl(1, TTY_IOCTL_SET_CURSOR, &home);
    printf("%s\x1b[2J", theme_background_bg());
    printf("theme loaded: %s\n", theme_active_path());
    return 0;
}
