#pragma once

#include <stdint.h>
#include <stddef.h>

#define O_RDONLY 0x0
#define O_WRONLY 0x1
#define O_RDWR   0x2
#define O_MODE   0x3
#define O_CREAT  0x4
#define O_TRUNC  0x8
#define O_APPEND 0x10

#define PATH_MAX    4096

typedef struct user_file{
    int permissions;
    size_t filesize;
    char fname[256];
} user_file_t;