#include <stdio.h>
#include <syscalls/open.h>
#include <syscalls/close.h>
#include <syscalls/read.h>
#include <syscalls/write.h>
#include <syscalls/getcwd.h>
#include <stdlib.h>
#include <fs/file.h>
#include <string.h>
#include <commands/commands.h>
#include <limits.h>

int cmd_cat(shell_cmd_t *cmd) {
    if (cmd->argc < 2) {
        printf("Bad Command Usage: cat <file>\n");
        return -1;
    }

    const char *filename = cmd->argv[1];
    char resolved[PATH_MAX];

    if (filename[0] == '/') {
        strncpy(resolved, filename, PATH_MAX - 1);
        resolved[PATH_MAX - 1] = '\0';
    } else {
        char cwd[PATH_MAX] = {0};
        if (!_getcwd(cwd, PATH_MAX)) {
            printf("cat: cannot get current directory\n");
            return -1;
        }

        size_t cwd_len = strlen(cwd);
        if (cwd_len == 1 && cwd[0] == '/') {
            snprintf(resolved, PATH_MAX, "/%s", filename);
        } else {
            snprintf(resolved, PATH_MAX, "%s/%s", cwd, filename);
        }
    }

    char buf[256];
    int fd = _open(resolved, O_RDONLY);
    if (fd < 0) {
        printf("cat: cannot open %s\n", resolved);
        return -2;
    }

    for (;;) {
        long r = _read(fd, buf, sizeof(buf));
        if (r == 0)
            break;
        if (r < 0) {
            printf("cat: read error (%ld)\n", r);
            break;
        }
        _write(1, buf, r);
    }

    printf("\n");
    _close(fd);
    return 0;
}
