#pragma once

#include <libc/types.h>
#include <libc/fs/file.h>

int open(const char *path, int flags, ...);
int fcntl(int fd, int cmd, ...);

#define F_GETFL 3
#define F_SETFL 4

#define O_NONBLOCK 0x20
