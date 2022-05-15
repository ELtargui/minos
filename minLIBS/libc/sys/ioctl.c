#include <stdarg.h>
#include <mink/syscall.h>
#include <sys/ioclt.h>
#include <errno.h>

int ioctl(int fd, unsigned long request, ...)
{
    va_list ap;
    va_start(ap, request);
    int a = va_arg(ap, int);
    int b = va_arg(ap, int);
    int c = va_arg(ap, int);

    int ret = 0;
    asm volatile(
        "push %%ebx; movl %2,%%ebx; int $0x80; pop %%ebx"
        : "=a"(ret)
        : "0"(SYS_ioctl), "r"(fd), "c"(request), "d"(a), "S"(b), "D"(c));
    va_end(ap);
    ERRNO_RET(ret, -1);
}
