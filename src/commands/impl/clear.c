#include <stdio.h>
#include <commands/commands.h>
#include <main.h>
#include <devices/console.h>
#include <syscalls/ioctl.h>

int cmd_clear(shell_cmd_t *cmd) {
    (void)cmd;
    tty_cursor_t home = { .x = 0, .y = 0 };
    (void)_ioctl(1, TTY_IOCTL_SET_CURSOR, &home);
    printf("\x1b[2J");
    return 0;
}