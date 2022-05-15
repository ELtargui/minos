#include <mink/syscall.h>
#include <poll.h>
#include <errno.h>

int poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
    int ret = 0;
    asm volatile(
        "push %%ebx; movl %2,%%ebx; int $0x80; pop %%ebx"
        : "=a"(ret)
        : "0"(SYS_poll), "r"(fds), "c"(nfds), "d"(timeout));
    ERRNO_RET(ret, -1);
}
