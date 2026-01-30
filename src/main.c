#include <stdio.h>
#include <string.h>
#include <main.h>
#include <ansii.h>
#include <fs/file.h>
#include <syscalls/stat.h>
#include <syscalls/getcwd.h>
#include <syscalls/close.h>
#include <syscalls/open.h>
#include <syscalls/read.h>
#include <syscalls/write.h>

void prompt(void) {
    char cwd[PATH_MAX];
    if (!_getcwd(cwd, sizeof(cwd))) strcpy(cwd, "?");
    printf("["RGB_FG(212, 44, 44) "shell" GRAY_FG "@bleed-kernel" RGB_FG(212, 44, 44)"%s" RESET"]# ", cwd);
}

// last bit is reserved for exec
void print_perms(int flags) {
    int mode = flags & O_MODE;
    char perms[3] = {'-', '-', '-'};

    if (mode == O_RDONLY) perms[0] = 'r';
    if (mode == O_WRONLY) perms[1] = 'w';
    if (mode == O_RDWR)   { perms[0] = 'r'; perms[1] = 'w'; }

    printf("%c%c%c", perms[0], perms[1], perms[2]);
}


int main(void) {
    char line[SHELL_MAX_LINE];
    shell_cmd_t cmd;
    
    char splash[256] = {0};
    int splashfd = _open("/initrd/etc/splash.txt", O_RDONLY);

    long r = _read(splashfd, splash, sizeof(splash) - 1);
    if (r > 0) {
        splash[r] = '\0';
        printf("%s\n", splash);
    }

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