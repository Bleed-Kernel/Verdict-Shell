#include <stdio.h>
#include <stdlib.h>
#include <fs/file.h>
#include <string.h>
#include <commands/commands.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>

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
        if (!getcwd(cwd, PATH_MAX)) {
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
    int fd = open(resolved, O_RDONLY);
    if (fd < 0) {
        printf("cat: cannot open %s\n", resolved);
        return -2;
    }

    for (;;) {
        long r = read(fd, buf, sizeof(buf));
        if (r == 0)
            break;
        if (r < 0) {
            printf("cat: read error (%ld)\n", r);
            break;
        }
        write(1, buf, r);
    }

    printf("\n");
    close(fd);
    return 0;
}
