#include <mink/types.h>
#include <mink/string.h>
#include <mink/debug.h>
#include <mink/mm.h>

void *memcpy(void *dest, const void *src, size_t n)
{
    assert(src);
    assert(dest);
    uint8_t *d = (uint8_t *)dest;
    uint8_t *s = (uint8_t *)src;

    for (size_t i = 0; i < n; i++)
    {
        d[i] = s[i];
    }

    return dest;
}

void *memset(void *dest, int c, size_t n)
{
    uint8_t *d = (uint8_t *)dest;

    for (size_t i = 0; i < n; i++)
    {
        d[i] = c;
    }

    return dest;
}

size_t strlen(const char *s)
{
    size_t i = 0;

    while (s[i])
        i++;

    return i;
}

size_t strnlen(const char *s, size_t len)
{
    size_t ret = 0;
    while (ret < len && s[ret])
        ret++;
    return ret;
}

char *strcpy(char *dest, const char *src)
{
    size_t i = 0;
    while (src[i])
    {
        dest[i] = src[i];
        i++;
    }
    dest[i] = 0;

    return dest;
}

char *strncpy(char *dest, char *src, size_t len)
{
    size_t i = 0;
    while (src[i] && i < len)
    {
        dest[i] = src[i];
        i++;
    }
    dest[i] = 0;

    return dest;
}

int strcmp(const char *s1, const char *s2)
{
    uint8_t *a = (uint8_t *)s1;
    uint8_t *b = (uint8_t *)s2;

    size_t i = 0;
    while (1)
    {

        if (a[i] == '\0' && b[i] == '\0')
            return 0;
        if (a[i] > b[i])
            return 1;
        if (a[i] < b[i])
            return -1;

        i++;
    }
    return -1;
}

int strncmp(const char *s1, const char *s2, size_t count)
{
    if (!count)
        return 0;

    while (--count && *s1 && *s1 == *s2)
    {
        s1++;
        s2++;
    }

    return *(uint8_t *)s1 - *(uint8_t *)s2;
}

char *strdup(const char *s)
{
    assert(s);
    char *_s = malloc(strlen(s) + 1);
    strcpy(_s, s);
    return _s;
}

char *strndup(const char *s, size_t size)
{
    assert(s);
    int len = strnlen(s, size);
    char *ret = malloc(len + 1);

    memcpy(ret, s, len);
    ret[len] = 0;
    return ret;
}
