#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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
    memset(out, 0, size);

    if (cmd->argc < 2 || strlen(cmd->argv[1]) == 0) {
        if (!_getcwd(out, size))
            return -1;
        out[size - 1] = '\0';
        return 0;
    }

    const char *arg = cmd->argv[1];

    // Absolute path case
    if (arg[0] == '/') {
        strncpy(out, arg, size - 1);
        out[size - 1] = '\0';
        return 0;
    }

    // Relative path case
    char cwd[PATH_MAX];
    memset(cwd, 0, sizeof(cwd));

    if (!_getcwd(cwd, sizeof(cwd)))
        return -1;
    cwd[sizeof(cwd) - 1] = '\0';

    size_t len = strlen(cwd);
    // Add trailing slash if missing
    if (len > 0 && len < sizeof(cwd) - 1 && cwd[len - 1] != '/') {
        cwd[len] = '/';
        cwd[len + 1] = '\0';
    }

    // copy CWD then append the argument
    strncpy(out, cwd, size - 1);
    out[size - 1] = '\0';

    strncat(out, arg, size - strlen(out) - 1);
    out[size - 1] = '\0'; // just in case lol

    return 0;
}

static void sort_entries(dirent_t *entries, size_t count) {
    for (size_t i = 0; i < count; i++) {
        for (size_t j = 0; j < count - i - 1; j++) {
            if (strcmp(entries[j].name, entries[j + 1].name) > 0) {
                dirent_t temp = entries[j];
                entries[j] = entries[j + 1];
                entries[j + 1] = temp;
            }
        }
    }
}


int cmd_ls(shell_cmd_t *cmd) {
    char *path = (char *)malloc(PATH_MAX);
    if (!path) {
        printf("OUT OF MEMORY\n");
        return -1;
    }

    memset(path, 0, PATH_MAX);
    if (build_ls_path(path, PATH_MAX, cmd) < 0) {
        printf("ls: failed to resolve path\n");
        free(path); 
        return -1;
    }
    path[PATH_MAX - 1] = '\0';

    int fd = _open(path, O_RDONLY);
    if (fd < 0) {
        printf("ls: cannot open %s\n", path);
        free(path);
        return -1;
    }

    dirent_t entries[256];
    memset(entries, 0, sizeof(entries));

    size_t count = 0;
    while (count < 256 && _readdir(fd, count, &entries[count]) == 0) {
        count++;
    }

    sort_entries(entries, count);

    for (size_t i = 0; i < count; i++) {
        const char *color = (entries[i].type == 0) ? theme_secondary_fg() : theme_primary_fg();
        
        char name[PATH_MAX]; 
        memset(name, 0, sizeof(name));

        strncpy(name, entries[i].name, PATH_MAX - 2); 
        name[PATH_MAX - 2] = '\0'; 
        
        if (entries[i].type == 0) {
            strncat(name, "/", PATH_MAX - strlen(name) - 1);
        }

        name[PATH_MAX - 1] = '\0';

        printf("%s%s%s    ", color, name, RESET);
    }

    printf("\n");

    _close(fd);
    free(path); 
    return 0;
}