#include <mink/syscall.h>
#include <sys/wait.h>
#include <errno.h>

pid_t waitpid(pid_t pid, int *stat_loc, int options)
{
    int ret = 0;
    asm volatile(
        "push %%ebx; movl %2,%%ebx; int $0x80; pop %%ebx"
        : "=a"(ret)
        : "0"(SYS_waitpid), "r"(pid), "c"(stat_loc), "d"(options));
    ERRNO_RET(ret, -1);
}

pid_t wait(int *stat_loc)
{
    return waitpid(-1, stat_loc, 0);
}
