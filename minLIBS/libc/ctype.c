#include <ctype.h>
int toascii(int);
int tolower(int);
int toupper(int);

int isalnum(int c)
{
    return isalpha(c) || isdigit(c);
}
int isalpha(int c)
{
    return isupper(c) || islower(c);
}
int isascii(int c)
{
    return (unsigned)c < 128;
}
int isblank(int c)
{
    return c == ' ' || c == '\t';
}
int iscntrl(int c)
{
    return 0 <= c && c < 32;
}

int isdigit(int c)
{
    return '0' <= c && c <= '9';
}

int isgraph(int c)
{
    return '!' <= c && c <= '~';
}

int islower(int c)
{
    return 'a' <= c && c <= 'z';
}

int isprint(int c)
{
    return isgraph(c) || c == ' ';
}

int ispunct(int c)
{
    return isprint(c) && c != ' ' && !isalnum(c);
}

int isspace(int c)
{
    return c == '\t' || c == '\n' || c == '\v' || c == '\f' || c == '\r' || c == ' ';
}

int isupper(int c)
{
    return 'A' <= c && c <= 'Z';
}

int isxdigit(int c)
{
    if (isdigit(c))
        return 1;
    if ('a' <= c && c <= 'f')
        return 1;
    if ('A' <= c && c <= 'F')
        return 1;
    return 0;
}

int toascii(int c)
{
    return c & 127;
}

int tolower(int c)
{
    if (c >= 'A' && c <= 'Z')
        return c | 0x20;
    return c;
}

int toupper(int c)
{
    if (c >= 'a' && c <= 'z')
        return c & ~0x20;
    return c;
}