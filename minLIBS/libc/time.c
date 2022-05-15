#include <time.h>
#include <mink/syscall.h>
#include <errno.h>

int nanosleep(const struct timespec *req, struct timespec *rem)
{
    int ret = 0;
    asm volatile(
        "push %%ebx; movl %2,%%ebx; int $0x80; pop %%ebx"
        : "=a"(ret)
        : "0"(SYS_nanosleep), "r"(req), "c"(rem));
    ERRNO_RET(ret, -1);
}
