#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <main.h>

int cmd_export(shell_cmd_t *cmd) {
    if (cmd->argc < 2) {
        for (char **e = environ; *e; e++)
            printf("export %s\n", *e);
        return 0;
    }

    for (int i = 1; i < cmd->argc; i++) {
        const char *eq = strchr(cmd->argv[i], '=');
        if (!eq)
            continue;

        char name[128];
        size_t len = (size_t)(eq - cmd->argv[i]);
        if (len == 0 || len >= sizeof(name)) {
            printf("export: bad name in %s\n", cmd->argv[i]);
            return -1;
        }
        memcpy(name, cmd->argv[i], len);
        name[len] = '\0';
        setenv(name, eq + 1, 1);
    }
    return 0;
}

int cmd_unset(shell_cmd_t *cmd) {
    for (int i = 1; i < cmd->argc; i++)
        unsetenv(cmd->argv[i]);
    return 0;
}

int cmd_env(shell_cmd_t *cmd) {
    (void)cmd;
    for (char **e = environ; *e; e++)
        printf("%s\n", *e);
    return 0;
}

int cmd_true(shell_cmd_t *cmd) {
    (void)cmd;
    return 0;
}

int cmd_false(shell_cmd_t *cmd) {
    (void)cmd;
    return -1;
}
