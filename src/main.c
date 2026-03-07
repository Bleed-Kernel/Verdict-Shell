#include <stdio.h>
#include <string.h>
#include <main.h>
#include <ansii.h>
#include <fs/file.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <syscalls/close.h>
#include <syscalls/open.h>
#include <syscalls/dup2.h>
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

static void shell_isolate_tty_session(void) {
    uint32_t current_index = 0;
    if (_ioctl(1, TTY_IOCTL_GET_INDEX, &current_index) < 0)
        return;

    uint32_t new_index = current_index;
    if (_ioctl(1, TTY_IOCTL_CREATE, &new_index) < 0)
        return;
    if (new_index == current_index)
        return;

    char tty_path[16];
    snprintf(tty_path, sizeof(tty_path), "/dev/tty%u", new_index);
    int tty_fd = (int)_open(tty_path, O_RDWR);
    if (tty_fd < 0)
        return;

    if (_dup2(tty_fd, 1) < 0 || _dup2(tty_fd, 2) < 0) {
        (void)_close((uint64_t)tty_fd);
        return;
    }

    (void)_close((uint64_t)tty_fd);
    (void)_ioctl(1, TTY_IOCTL_SET_ACTIVE, &new_index);
}

static void shell_set_nonblock(int fd) {
    if (fcntl(fd, F_SETFL, O_NONBLOCK) == 0)
        return;

    uint32_t flags = TTY_NONBLOCK;
    (void)_ioctl(fd, TTY_IOCTL_SET_FLAGS, &flags);
}

static void shell_set_line_editor_tty_mode(int fd) {
    struct termios t;
    if (tcgetattr(fd, &t) == 0) {
        t.c_lflag &= ~(ECHO | ICANON);
        t.c_cc[VMIN] = 0;
        t.c_cc[VTIME] = 0;
        (void)tcsetattr(fd, TCSANOW, &t);
        return;
    }

    uint32_t flags = 0;
    if (_ioctl(fd, TTY_IOCTL_GET_FLAGS, &flags) == 0) {
        flags |= TTY_NONBLOCK;
        flags &= ~(TTY_ECHO | TTY_CANNONICAL);
        (void)_ioctl(fd, TTY_IOCTL_SET_FLAGS, &flags);
    } else {
        flags = TTY_NONBLOCK;
        (void)_ioctl(fd, TTY_IOCTL_SET_FLAGS, &flags);
    }
}

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
    shell_isolate_tty_session();

    shell_set_nonblock(0);
    shell_set_nonblock(1);
    shell_set_nonblock(2);

    shell_set_line_editor_tty_mode(1);
    shell_set_line_editor_tty_mode(2);

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
