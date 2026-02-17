#pragma once

#include <libc/types.h>
#include <libc/stddef.h>

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

ssize_t read(int fd, void *buf, size_t len);
ssize_t write(int fd, const void *buf, size_t len);
int close(int fd);
off_t lseek(int fd, off_t offset, int whence);
int chdir(const char *path);
char *getcwd(char *buf, size_t size);
int isatty(int fd);
