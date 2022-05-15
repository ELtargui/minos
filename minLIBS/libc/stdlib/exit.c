#include <stdlib.h>

extern void libc_exit(int status);

void exit(int status)
{
    libc_exit(status);
    __builtin_unreachable();
}
