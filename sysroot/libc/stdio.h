#pragma once
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

#define stdin   0
#define stdout  1
#define stderr  2

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define FILE_READ   0x01
#define FILE_WRITE  0x02
#define FILE_APPEND 0x04

typedef struct {
    int     fd;

    unsigned char *buffer;
    size_t  buf_size;
    size_t  buf_pos;
    size_t  buf_len;

    int     flags;
    int     eof;
    int     error;
} FILE;

int printf(const char *fmt, ...);
int fprintf(int fd, const char *fmt, ...);
int vfprintf(int fd, const char *fmt, va_list ap);
int vsnprintf(char *buf, size_t size, const char *fmt, va_list ap);
int snprintf(char *buf, size_t size, const char *fmt, ...);

FILE *fopen(const char *path, const char *mode);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
int fflush(FILE *stream);
int fclose(FILE *stream);

int tell(void* handle);
