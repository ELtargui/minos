#pragma once

#include <mink/types.h>


void *memcpy(void *dest, const void *src, size_t n);
void *memset(void *dest, int c, size_t n);
size_t strlen(const char *s);
size_t strnlen(const char *s, size_t len);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, char *src, size_t len);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t count);
char *strdup(const char *s);
char *strndup(const char *s, size_t size);
