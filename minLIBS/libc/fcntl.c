#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>
#include <mink/syscall.h>

int creat(const char *path, mode_t mode)
{
    return open(path, O_WRONLY | O_CREAT | O_TRUNC, mode);
}

int fcntl(int fildes, int cmd, ...)
{
    ERRNO_RET(-ENOSYS, -1);
}

int open(const char *path, int oflag, ...)
{
    va_list ap;
    int mode = 0;
    va_start(ap, oflag);
    if (oflag & O_CREAT)
        mode = va_arg(ap, int);
    va_end(ap);
    int ret = 0;
    asm volatile(
        "push %%ebx; movl %2,%%ebx; int $0x80; pop %%ebx"
        : "=a"(ret)
        : "0"(SYS_open), "r"(path), "c"(oflag), "d"(mode));
    ERRNO_RET(ret, -1);
}

int openat(int fd, const char *path, int oflag, ...)
{
    ERRNO_RET(-ENOSYS, -1);
}
