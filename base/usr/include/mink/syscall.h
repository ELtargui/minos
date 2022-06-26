#pragma once

typedef enum SYS_call
{
    SYS_none = 0,
    SYS_exit,
    SYS_open,
    SYS_close,
    SYS_read,
    SYS_write,
    SYS_fork,
    SYS_execve,
    SYS_sched_yield,
    SYS_waitpid,
    SYS_gettid,
    SYS_getpid,
    SYS_getppid,
    SYS_mmap,
    SYS_munmap,
    SYS_thread,
    SYS_thread_exit,
    SYS_thread_detach,
    SYS_thread_join,
    SYS_nanosleep,
    SYS_lseek,
    SYS_ioctl,
    SYS_poll,
    SYS_openpty,
    SYS_shm_open,
    SYS_shm_unlink,
    SYS_truncate,
    SYS_ftruncate,
    SYS_stat,
    SYS_fstat,
    SYS_lstat,
    SYS_pipe2,
    SYS_dup,
    SYS_dup3,
} SYS_call_t;
