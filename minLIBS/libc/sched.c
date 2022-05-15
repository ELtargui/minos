#include <mink/syscall.h>
#include <sched.h>

int sched_yield(void)
{
    int ret = 0;

    asm volatile(
        "int $0x80"
        : "=a"(ret)
        : "0"(SYS_sched_yield));
    return ret;
}
