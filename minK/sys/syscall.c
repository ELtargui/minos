#include <mink/types.h>
#include <mink/debug.h>
#include <mink/mm.h>
#include <mink/fs.h>
#include <mink/fsevent.h>
#include <mink/process.h>
#include <mink/region.h>
#include <mink/string.h>
#include <mink/syscall.h>
#include <errno.h>

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

int sys_mmap(struct s_mmap *args)
{
    return process_mmap(current_process(), args->addr, args->len, args->prot,
                        args->flags, args->fd, args->offset, args->name);
}

int sys_munmap(void *addr, size_t size)
{
    return process_unmap((uintptr_t)addr, size);
}

int sys_exit(int status)
{
    process_exit(current_process(), status);
    return -1;
}

int sys_open(const char *filename, int flags, int mode)
{
    return file_open(filename, flags, mode);
}

int sys_close(int fd)
{
    return file_close(current_process(), fd);
}

int sys_read(int fd, void *buf, size_t size)
{
    return file_read(current_process(), fd, buf, size);
}

int sys_write(int fd, void *buf, size_t size)
{
    return file_write(current_process(), fd, buf, size);
}

int sys_ioctl(int fd, uint32_t cmd, void *a, void *b)
{
    return file_ioctl(fd, cmd, a, b);
}

int sys_fork()
{
    return fork();
}

int sys_execve(const char *filename, const char **argv, const char **envp)
{
    process_t *process = current_process();
    assert(process);

    list_delete_all(process->argv, free);
    list_delete_all(process->envp, free);
    int i = 0;
    while (argv[i])
    {
        list_append(process->argv, strdup(argv[i]));
        i++;
    }

    if (envp)
    {
        assert(0);
    }

    return execve(filename);
}

int sys_thread(uintptr_t funp, uintptr_t argp, uintptr_t stack, size_t stacksize)
{
    assert(funp);
    assert(stack);
    return spawn_thread(funp, argp, stack, stacksize);
}

int sys_thread_join(int id, void **retvp)
{
    thread_t *thread = thread_from_id(current_process(), id);

    if (!thread)
    {
        return -ESRCH;
    }
    int e = thread_join(thread);
    if (retvp)
        *retvp = current_thread()->joinee_retval;
    return e;
}

int sys_thread_detach(int id)
{
    thread_t *thread = thread_from_id(current_process(), id);
    if (!thread)
        return -ESRCH;

    if (!thread->joinable)
        return -EINVAL;
    thread->joinable = 0;
    return 0;
}

int sys_thread_exit(void *retval)
{
    dbgln("exit ret:%p", retval);
    current_thread()->exit_value = retval;
    if (current_thread()->joiner)
    {
        if (current_thread()->joiner)
            current_thread()->joiner->joinee_retval = retval;
        sched_run(current_thread()->joiner);
    }

    thread_exit_self();
    assert(0);
    return -1;
}

int sys_sched_yield()
{
    sched_yield();
    return 0;
}

int sys_waitpid(int pid, int *stat_loc, int options)
{
    if (stat_loc)
        assert(stat_loc);
    return waitpid(pid, stat_loc, options);
}

int sys_gettid()
{
    return current_thread()->id;
}

int sys_getpid()
{
    process_t *p = current_process();
    assert(p);
    return p->pid;
}

int sys_getppid()
{
    process_t *p = current_process();
    assert(p);
    process_t *pp = p->parent;
    return pp ? pp->pid : 0;
}

int sys_poll(pollfd_t *fds, int nfds, int timeout)
{
    return poll(fds, nfds, timeout);
}

int sys_nanosleep(const struct timespec *req, struct timespec *rem)
{
    return nanosleep(req, rem);
}

int sys_shm_open(const char *name, int oflag, int mode)
{
    return shm_open(name, oflag, mode);
}

int sys_shm_unlink(const char *name)
{
    return shm_unlink(name);
}

int sys_truncate(const char *name, off_t length)
{
    return file_truncate(name, length);
}
int sys_ftruncate(int fd, off_t length)
{
    return file_fdtruncate(fd, length);
}

int sys_stat(const char *file, stat_t *stat)
{
    fsnode_t *f = fs_open(file, 0, 0);
    if (!f)
        return -ENOENT;

    int r = fsnode_stat(f, stat);
    fsnode_close(f);

    return r;
}

int sys_fstat(int fd, stat_t *stat)
{
    filedescriptor_t *f = fd_get(current_process(), fd);
    if (!f)
    {
        return -EBADF;
    }

    return fsnode_stat(f->node, stat);
}

int sys_linkstat(const char *file, stat_t *stat)
{
    (void)file;
    (void)stat;
    return -1;
}

int sys_lseek(int fd, off_t offset, int whence)
{
    filedescriptor_t *file = fd_get(current_process(), fd);
    if (!file)
    {
        return -EBADF;
    }

    switch (whence)
    {
    case SEEK_SET:
        file->offset = offset;
        break;
    case SEEK_CUR:
        file->offset += offset;
        break;
    case SEEK_END:
        file->offset = file->node->length + offset;
        break;
    default:
        return -EINVAL;
    }
    return file->offset;
}

static int (*sys_table[])() = {
    [SYS_exit] = sys_exit,
    [SYS_open] = sys_open,
    [SYS_close] = sys_close,
    [SYS_mmap] = sys_mmap,
    [SYS_munmap] = sys_munmap,
    [SYS_gettid] = sys_gettid,
    [SYS_getpid] = sys_getpid,
    [SYS_getppid] = sys_getppid,
    [SYS_read] = sys_read,
    [SYS_write] = sys_write,
    [SYS_fork] = sys_fork,
    [SYS_waitpid] = sys_waitpid,
    [SYS_execve] = sys_execve,
    [SYS_thread] = sys_thread,
    [SYS_sched_yield] = sys_sched_yield,
    [SYS_thread_exit] = sys_thread_exit,
    [SYS_thread_detach] = sys_thread_detach,
    [SYS_thread_join] = sys_thread_join,
    [SYS_poll] = sys_poll,
    [SYS_nanosleep] = sys_nanosleep,
    [SYS_ioctl] = sys_ioctl,
    [SYS_shm_open] = sys_shm_open,
    [SYS_shm_unlink] = sys_shm_unlink,
    [SYS_truncate] = sys_truncate,
    [SYS_ftruncate] = sys_ftruncate,
    [SYS_stat] = sys_stat,
    [SYS_fstat] = sys_fstat,
    [SYS_lstat] = sys_linkstat,
    [SYS_lseek] = sys_lseek,
};

void sys_handler(regs_t *r)
{
    current_thread()->cpu.r = r;

    uint32_t sys = r->eax;
    if (sys >= sizeof(sys_table) / sizeof(void *))
    {
        dbgln("inavalide syscall number [%d]", sys);
        r->eax = -1;
        assert(0);
        return;
    }
    if ((uintptr_t)(sys_table[sys]) == 0)
    {
        dbgln("sys:%d", sys);
        assert(0);
    }
    r->eax = sys_table[sys](r->ebx, r->ecx, r->edx, r->esi, r->edi);
}

void syscall_install()
{
    install_isr_handler(128, sys_handler);
}
