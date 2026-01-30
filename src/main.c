#include <stdio.h>
#include <string.h>
#include <main.h>
#include <ansii.h>
#include <syscalls/getcwd.h>
#include <syscalls/close.h>
#include <syscalls/open.h>
#include <syscalls/read.h>

void prompt(void) {
    char cwd[PATH_MAX];
    if (!_getcwd(cwd, sizeof(cwd))) strcpy(cwd, "?");
    printf("["RGB_FG(212, 44, 44) "shell" GRAY_FG "@bleed-kernel" RGB_FG(212, 44, 44)"%s" RESET"]# ", cwd);
}

int main(void) {
    char line[SHELL_MAX_LINE];
    shell_cmd_t cmd;
    
    char splash[256] = {0};
    int splashfd = _open("/initrd/etc/splash.txt", O_RDONLY);
    long r = _read(splashfd, splash, sizeof(splash));
    if (r > 0) printf("%s\n", splash);
    _close(splashfd);

    if (path_init() < 0) {
        printf(LOG_WARN "failed to load PATH\n");
    }

    for (;;) {
        prompt();

        if (shell_read_line(line, sizeof(line)) <= 0)
            continue;

        if (shell_parse(line, &cmd) < 0)
            continue;

        shell_execute(&cmd);
    }
}