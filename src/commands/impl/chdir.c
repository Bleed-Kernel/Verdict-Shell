#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <main.h>
#include <unistd.h>
#include <fs/file.h>

int cmd_chdir(shell_cmd_t *cmd) {
    if (cmd->argc < 2) {
        printf("Usage: cd <path>\n");
        return -1;
    }

    char new_path[PATH_MAX] = {0};
    const char *arg = cmd->argv[1];

    if (strcmp(arg, "..") == 0) {
        char cwd[PATH_MAX] = {0};
        if (!getcwd(cwd, PATH_MAX)) {
            printf("cd: failed to get current directory\n");
            return -1;
        }

        size_t len = strlen(cwd);
        if (len == 1 && cwd[0] == '/') {
            strcpy(new_path, "/");  // this is why we root
        } else {
            size_t i = len - 1;
            while (i > 0 && cwd[i] == '/') i--;
            while (i > 0 && cwd[i] != '/') i--;
            if (i == 0) i = 1;
            strncpy(new_path, cwd, i);
            new_path[i] = '\0';
        }
    } else {
        // normal path
        strncpy(new_path, arg, PATH_MAX - 1);
        new_path[PATH_MAX - 1] = '\0';
    }

    if (chdir(new_path) < 0) {
        printf("cd: failed to change directory to %s\n", new_path);
        return -1;
    }

    return 0;
}
