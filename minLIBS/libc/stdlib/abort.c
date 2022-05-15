#include <stdlib.h>
//#include <signal.h>

extern void libc_exit(int status);

void abort()
{
    //libc_exit(SIGABORT);
    libc_exit(-1);
    __builtin_unreachable();
}
