#include <errno.h>
#include <sys/stat.h>
#include <mink/syscall.h>

int stat(const char *pathname, struct stat *statbuf)
{
    int ret = 0;
    asm volatile(
        "push %%ebx; movl %2,%%ebx; int $0x80; pop %%ebx"
        : "=a"(ret)
        : "0"(SYS_stat), "r"(pathname), "c"(statbuf));
    ERRNO_RET(ret, -1);
}

int fstat(int fd, struct stat *statbuf)
{
    int ret = 0;
    asm volatile(
        "push %%ebx; movl %2,%%ebx; int $0x80; pop %%ebx"
        : "=a"(ret)
        : "0"(SYS_fstat), "r"(fd), "c"(statbuf));
    ERRNO_RET(ret, -1);
}

int lstat(const char *pathname, struct stat *statbuf)
{
    int ret = 0;
    asm volatile(
        "push %%ebx; movl %2,%%ebx; int $0x80; pop %%ebx"
        : "=a"(ret)
        : "0"(SYS_lstat), "r"(pathname), "c"(statbuf));
    ERRNO_RET(ret, -1);
}
