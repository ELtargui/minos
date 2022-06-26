#include <stdlib.h>
#include <ctype.h>

int atoi(const char *a)
{
    int i = 0;
    while (isdigit(*a))
    {
        i = i * 10 + *a++ - '0';
    }
    return i;
}
