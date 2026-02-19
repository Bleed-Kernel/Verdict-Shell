#include <stdio.h>
#include <string.h>
#include <syscalls/open.h>
#include <syscalls/readdir.h>
#include <syscalls/close.h>
#include <syscalls/getcwd.h>
#include <fs/file.h>
#include <main.h>
#include <ansii.h>
#include <limits.h>
#include <theme.h>

static int build_ls_path(char *out, size_t size, shell_cmd_t *cmd) {
    if (cmd->argc < 2 || strlen(cmd->argv[1]) == 0) {
        if (!_getcwd(out, size))
            return -1;
        return 0;
    }

    const char *arg = cmd->argv[1];

    // root case here
    if (arg[0] == '/') {
        strncpy(out, arg, size - 1);
        out[size - 1] = 0;
        return 0;
    }

    char cwd[PATH_MAX];
    if (!_getcwd(cwd, sizeof(cwd)))
        return -1;

    size_t len = strlen(cwd);
    if (len > 1 && cwd[len - 1] != '/')
        cwd[len++] = '/';

    strncpy(out, cwd, size - 1);
    out[size - 1] = 0;

    strncat(out, arg, size - strlen(out) - 1);
    return 0;
}

int cmd_ls(shell_cmd_t *cmd) {
    char path[PATH_MAX];

    if (build_ls_path(path, sizeof(path), cmd) < 0) {
        printf("ls: failed to resolve path\n");
        return -1;
    }

    int fd = _open(path, O_RDONLY);
    if (fd < 0) {
        printf("ls: cannot open %s\n", path);
        return -1;
    }

    dirent_t ent;
    for (size_t i = 0; _readdir(fd, i, &ent) == 0; i++) {
        const char *color = (ent.type == 0) ? theme_secondary_fg() : theme_primary_fg();
        char name[PATH_MAX] = {0};
        strcpy(name, ent.name);
        if (ent.type == 0) strcat(name, "/");
        printf("%s%-10s%s", color, name, RESET);
    }

    printf("\n");
    _close(fd);
    return 0;
}
