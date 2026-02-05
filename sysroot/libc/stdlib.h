#pragma once
#include <stdint.h>
#include <stddef.h>

int atoi(const char *s);

void free(void* ptr);
void* malloc(size_t size);
void* realloc(void* ptr, size_t size);
void* calloc(size_t nmemb, size_t size);

int abs(int x);