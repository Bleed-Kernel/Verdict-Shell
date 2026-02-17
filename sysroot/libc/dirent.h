#pragma once

#include <libc/types.h>

struct dirent {
    ino_t d_ino;
    unsigned char d_type;
    char d_name[256];
};
typedef struct dirent dirent;

typedef struct {
    int fd;
    unsigned long index;
    struct dirent entry;
} DIR;

DIR *opendir(const char *name);
struct dirent *readdir(DIR *dirp);
int closedir(DIR *dirp);
void rewinddir(DIR *dirp);
