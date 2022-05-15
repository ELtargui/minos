#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <mink/syscall.h>

struct s_mmap
{
    unsigned int addr;
    size_t len;
    int prot;
    int flags;
    int fd;
    off_t offset;
    char *name;
} __attribute__((packed));

void *mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off)
{

    static struct s_mmap mmap_args;

    if (len % 0x1000)
    {
        len &= 0xfffff000;
        len += 0x1000;
    }
    mmap_args.addr = (unsigned int)addr;
    mmap_args.len = len;
    mmap_args.prot = prot;
    mmap_args.flags = flags;
    mmap_args.fd = fildes;
    mmap_args.offset = off;
    mmap_args.name = NULL;

    int ret = 0;
    asm volatile(
        "push %%ebx; movl %2,%%ebx; int $0x80; pop %%ebx"
        : "=a"(ret)
        : "0"(SYS_mmap), "r"(&mmap_args));

    if (ret < 0)
    {
        errno = -ret;
        return MAP_FAILED;
    }

    return (void *)ret;
}

int mprotect(void *addr, size_t len, int prot)
{
    ERRNO_RET(-ENOSYS, -1);
}

int munmap(void *addr, size_t len)
{
    int ret = 0;
    asm volatile(
        "push %%ebx; movl %2,%%ebx; int $0x80; pop %%ebx"
        : "=a"(ret)
        : "0"(SYS_munmap), "r"(addr), "c"(len));
    ERRNO_RET(ret, -1);
}

int posix_madvise(void *addr, size_t len, int advice)
{
    ERRNO_RET(-ENOSYS, -1);
}

int shm_open(const char *name, int oflag, mode_t mode)
{
    int ret = 0;
    asm volatile(
        "push %%ebx; movl %2,%%ebx; int $0x80; pop %%ebx"
        : "=a"(ret)
        : "0"(SYS_shm_open), "r"(name), "c"(oflag), "d"(mode));
    ERRNO_RET(ret, -1);
}

int shm_unlink(const char *name)
{
    int ret = 0;
    asm volatile(
        "push %%ebx; movl %2,%%ebx; int $0x80; pop %%ebx"
        : "=a"(ret)
        : "0"(SYS_shm_unlink), "r"(name));
    ERRNO_RET(ret, -1);
}
