#include <stdio.h>

#include <commands/commands.h>
#include <theme.h>

int cmd_theme(shell_cmd_t *cmd) {
    if (cmd->argc < 2) {
        printf("active theme: %s\n", theme_active_path());
        printf("usage: theme <path>\n");
        return 0;
    }

    const char *path = cmd->argv[1];
    if (theme_load(path) < 0) {
        printf("theme: failed to load %s\n", path);
        return -1;
    }

    printf("%s\x1b[J", theme_background_bg());
    printf("theme loaded: %s\n", theme_active_path());
    return 0;
}
