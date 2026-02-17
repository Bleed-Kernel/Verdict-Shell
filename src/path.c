#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fs/file.h>
#include <main.h>

static char paths[SHELL_MAX_PATHS][SHELL_MAX_PATH_LEN];
static int path_count = 0;

int path_init(void) {
    int fd = open("/initrd/etc/path", O_RDONLY);
    if (fd < 0)
        return -1;

    char buf[512];
    int n = read(fd, buf, sizeof(buf) - 1);
    close(fd);

    if (n <= 0)
        return -1;

    buf[n] = 0;

    char *p = buf;
    while (*p && path_count < SHELL_MAX_PATHS) {
        char *start = p;
        while (*p && *p != '\n')
            p++;

        size_t len = p - start;
        if (len > 0 && len < SHELL_MAX_PATH_LEN) {
            memcpy(paths[path_count], start, len);
            paths[path_count][len] = 0;
            path_count++;
        }

        if (*p)
            p++;
    }

    return 0;
}

const char *path_resolve(const char *name) {
    static char full[SHELL_MAX_PATH_LEN * 2];

    for (int i = 0; i < path_count; i++) {
        snprintf(full, sizeof(full), "%s/%s", paths[i], name);
        return full;
    }

    return NULL;
}
