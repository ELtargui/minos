#include <string.h>
#include <stdlib.h>

void *memchr(const void *s, int c, size_t n)
{
    unsigned char *src = (unsigned char *)s;
    for (size_t i = 0; i < n; i++)
    {
        if (src[i] == (unsigned char)c)
        {
            return &src[i];
        }
    }

    return NULL;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
    unsigned char *a = (unsigned char *)s1;
    unsigned char *b = (unsigned char *)s2;
    for (size_t i = 0; i < n; i++)
    {
        if (a[i] == b[i])
            continue;
        if (a[i] > b[i])
        {
            return 1;
        }
        else
        {
            return -1;
        }
    }

    return 0;
}

void *memcpy(void *s1, const void *s2, size_t n)
{
    void *ret = s1;
    asm volatile(
        "rep movsb"
        : "+D"(s1), "+S"(s2), "+c"(n)::"memory");
    return ret;
}

void *memmove(void *s1, const void *s2, size_t n)
{
    if (s1 < s2)
        return memcpy(s1, s2, n);

    unsigned char *pd = (unsigned char *)s1;
    const unsigned char *ps = (const unsigned char *)s2;
    for (pd += n, ps += n; n--;)
        *--pd = *--ps;
    return pd;
}

void *memset(void *dest, int c, size_t n)
{
    void *ret = dest;
    asm volatile(
        "rep stosb\n"
        : "=D"(dest), "=c"(n)
        : "0"(dest), "1"(n), "a"(c)
        : "memory");
    return ret;
}

char *strcat(char *s1, const char *s2)
{
    size_t len = strlen(s1);
    int i = 0;
    while (s2[i])
    {
        s1[len + i] = s2[i];
        i++;
    }
    s1[len + i] = 0;
    return s1;
}

char *strncat(char *s1, const char *s2, size_t n)
{
    size_t len = strlen(s1);
    size_t i = 0;
    while (s2[i] && i < n)
    {
        s1[len + i] = s2[i];
        i++;
    }
    s1[len + i] = 0;
    return s1;
}

size_t strlen(const char *s)
{
    size_t len = 0;
    while (s[len])
        len++;
    return len;
}

size_t strnlen(const char *s, size_t maxlen)
{
    size_t len = 0;
    while (s[len] && len < maxlen)
        len++;
    return len;
}

char *strcpy(char *dest, const char *src)
{
    int i = 0;
    while ((dest[i] = src[i]))
        i++;
    return dest;
}

char *strncpy(char *dest, const char *src, size_t n)
{
    size_t i = 0;
    while (i < n)
    {
        dest[i] = src[i];
        if (!src[i])
            break;
        i++;
    }

    while (i < n)
        dest[i] = 0;
    return dest;
}

int strcmp(const char *s1, const char *s2)
{
    int i = 0;
    while (s1[i] == s2[i])
    {
        i++;
    }

    return (unsigned char)s1[i] - (unsigned char)s2[i];
}

int strncmp(const char *s1, const char *s2, size_t n)
{
    size_t i = 0;
    while ((s1[i] == s2[i]) && i < n)
    {
        i++;
    }

    return (unsigned char)s1[i] - (unsigned char)s2[i];
}

char *strchr(const char *s, int c)
{
    while (1)
    {
        if (*s == c)
            return (char *)s;
        if (!*s)
            return NULL;
        s++;
    }
}
char *strstr(const char *s1, const char *s2)
{
    char *ret = NULL;
    int len = strlen(s2);
    if (!len)
        return NULL;
    int i = 0, j = 0;
    while (j < len)
    {
        if (s1[i] == s2[j])
        {
            if (!ret)
                ret = (char *)&s1[i];
            j++;
        }
        else
        {
            j = 0;
            ret = NULL;
        }
        i++;
    }

    return ret;
}

char *strtok_r(char *s, const char *sep, char **state)
{
    return NULL;
}

char *strtok(char *s, const char *sep)
{
    static char *state;
    return strtok_r(s, sep, &state);
}

char *strsignal(int signum)
{
    return NULL;
}

char *strdup(const char *s)
{
    char *d = malloc(strlen(s) + 1);
    if (!d)
        return NULL;
    strcpy(d, s);
    return d;
}

char *strndup(const char *s, size_t n)
{
    int len = strnlen(s, n) + 1;
    char *d = malloc(len);
    if (!d)
        return NULL;
    memcpy(d, s, len);
    d[len] = 0;
    return d;
}
