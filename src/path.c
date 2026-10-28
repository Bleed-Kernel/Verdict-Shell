#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <main.h>

#define DATA_DIR "/mnt/data"

int path_init(void) {
    if (getenv("PATH"))
        return 0;

    int fd = open("/initrd/etc/path", O_RDONLY);
    if (fd < 0)
        return -1;

    char buf[512];
    int n = read(fd, buf, sizeof(buf) - 1);
    close(fd);

    if (n <= 0)
        return -1;
    buf[n] = 0;

    while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r'))
        buf[--n] = 0;
    for (char *p = buf; *p; p++) {
        if (*p == '\n')
            *p = ':';
    }

    return setenv("PATH", buf, 1);
}

const char *path_resolve(const char *name) {
    static char full[SHELL_MAX_PATH_LEN * 2];
    const char *path = getenv("PATH");

    while (path && *path) {
        const char *end = strchr(path, ':');
        int len = end ? (int)(end - path) : (int)strlen(path);

        if (len > 0) {
            snprintf(full, sizeof(full), "%.*s/%s", len, path, name);

            int fd = open(full, O_RDONLY);
            if (fd >= 0) {
                close(fd);
                return full;
            }
        }

        path = end ? end + 1 : "";
    }

    DIR *dir = opendir(DATA_DIR);
    if (!dir)
        return NULL;

    struct dirent *e;
    while ((e = readdir(dir)) != NULL) {
        if (e->d_name[0] == '.' || e->d_type != DT_DIR)
            continue;

        snprintf(full, sizeof(full), DATA_DIR "/%s/%s", e->d_name, name);
        int fd = open(full, O_RDONLY);
        if (fd >= 0) {
            close(fd);
            closedir(dir);
            return full;
        }
    }
    closedir(dir);
    return NULL;
}
