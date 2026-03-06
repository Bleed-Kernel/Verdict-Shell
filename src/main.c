#include <stdio.h>
#include <string.h>
#include <main.h>
#include <ansii.h>
#include <fs/file.h>
#include <stdlib.h>
#include <syscalls/close.h>
#include <syscalls/getcwd.h>
#include <syscalls/write.h>
#include <syscalls/taskcount.h>
#include <syscalls/mapfb.h>
#include <syscalls/exit.h>
#include <syscalls/ioctl.h>
#include <graphics/display.h>
#include <commands/commands.h>
#include <devices/console.h>
#include <theme.h>

uint32_t shell_tty_flags = TTY_NONBLOCK;

void prompt(void) {
    char splash[256] = {0};
    FILE *f = fopen("/initrd/etc/hostname", "r");
    if (f) {
        size_t n = fread(splash, 1, sizeof(splash) - 1, f);
        splash[n] = '\0';
        fclose(f);
    }else{ // even if this fails its not the end of the world
        snprintf(splash, sizeof(splash), "shell");
    }

    char cwd[PATH_MAX];
    if (!_getcwd(cwd, sizeof(cwd))) strcpy(cwd, "?");
    printf("%s[%s%s%s@bleed-kernel%s%s%s]#%s ",
            theme_primary_fg(),
            theme_secondary_fg(), splash,
            theme_primary_fg(),
            theme_secondary_fg(), cwd, theme_primary_fg(),
            theme_text_fg()
        );
}

int main(void) {
    _ioctl(0, TTY_IOCTL_SET_FLAGS, &shell_tty_flags);
    _ioctl(1, TTY_IOCTL_SET_FLAGS, &shell_tty_flags);
    _ioctl(2, TTY_IOCTL_SET_FLAGS, &shell_tty_flags);
    theme_init();
    printf("%s", theme_background_bg());
    printf("\x1b[J");
    char line[SHELL_MAX_LINE];
    shell_cmd_t cmd;

    FILE *sf = fopen("/initrd/etc/splash.txt", "r");
    if (sf) {
        char buf[256];
        size_t n;

        while ((n = fread(buf, 1, sizeof(buf), sf)) > 0) {
            fwrite(buf, 1, n, stdout);
        }

        fclose(sf);
        printf("\n");
    }

    if (path_init() < 0) {
        printf(LOG_WARN "failed to load PATH\n");
    }

    if (shell_install_signal_handlers() < 0) {
        printf(LOG_WARN "failed to install SIGINT handler\n");
    }

    for (;;) {
        prompt();

        if (shell_read_line(line, sizeof(line)) <= 0)
            continue;

        if (shell_parse(line, &cmd) < 0)
            continue;

        shell_execute(&cmd);
    }

    _exit(0);
}
