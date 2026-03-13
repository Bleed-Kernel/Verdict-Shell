#include <stdio.h>
#include <string.h>
#include <main.h>
#include <ansii.h>
#include <fs/file.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <syscalls/open.h>
#include <syscalls/read.h>
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

#if defined(__has_include)
#if __has_include(<wm.h>)
#include <wm.h>
#define HAVE_WM 1
#endif
#endif

#ifndef HAVE_WM
#define HAVE_WM 0
#endif

#if HAVE_WM
static void wm_try_placeholder(int argc, char **argv) {
    uint64_t wm_pid = 0;
    wm_client c = {0};
    wm_window win = {0};

    if (wm_parse_args(argc, (const char **)argv, &wm_pid) < 0)
        return;
    if (wm_connect(&c, wm_pid) < 0)
        return;
    if (wm_create_window(&c, "Verdict Shell", 640, 400, &win) < 0)
        return;

    uint32_t *p = win.pixels;
    size_t total = (size_t)win.width * (size_t)win.height;
    for (size_t i = 0; i < total; ++i)
        p[i] = 0x00182233u;
}
#endif

static char g_hostname[256];

static void shell_load_hostname(void) {
    int fd;
    long n;

    memset(g_hostname, 0, sizeof(g_hostname));
    fd = (int)_open("/initrd/etc/hostname", O_RDONLY);
    if (fd < 0) {
        snprintf(g_hostname, sizeof(g_hostname), "shell");
        return;
    }

    n = _read((uint64_t)fd, g_hostname, sizeof(g_hostname) - 1);
    _close((uint64_t)fd);

    if (n <= 0) {
        snprintf(g_hostname, sizeof(g_hostname), "shell");
        return;
    }

    g_hostname[n] = '\0';
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
    tty_cursor_t cursor = {0};
    if (_ioctl(1, TTY_IOCTL_GET_CURSOR, &cursor) == 0 && cursor.x != 0) {
        printf("\n");
    }
    printf("\x1b[0m");

    char cwd[PATH_MAX];
    if (!_getcwd(cwd, sizeof(cwd))) strcpy(cwd, "?");
    printf("%s[%s%s%s@bleed-kernel%s%s%s]#%s ",
            theme_primary_fg(),
            theme_secondary_fg(), g_hostname,
            theme_primary_fg(),
            theme_secondary_fg(), cwd, theme_primary_fg(),
            theme_text_fg()
        );
}

int main(int argc, char **argv) {
#if HAVE_WM
    wm_try_placeholder(argc, argv);
#endif
    (void)argc;
    (void)argv;
    shell_set_nonblock(0);
    shell_set_nonblock(1);
    shell_set_nonblock(2);

    shell_set_line_editor_tty_mode(1);
    shell_set_line_editor_tty_mode(2);

    theme_init();
    shell_load_hostname();
    printf("\x1b[0m%s", theme_background_bg());
    printf("\x1b[2J");
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
