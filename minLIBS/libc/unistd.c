#include <mink/syscall.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <errno.h>

extern void libc_exit(int status);

void _exit(int status)
{
    libc_exit(status);
    __builtin_unreachable();
}

ssize_t read(int fd, void *buf, size_t size)
{
    int ret = 0;
    asm volatile(
        "push %%ebx; movl %2,%%ebx; int $0x80; pop %%ebx"
        : "=a"(ret)
        : "0"(SYS_read), "r"(fd), "c"(buf), "d"(size));
    ERRNO_RET(ret, -1);
}

ssize_t write(int fd, const void *buf, size_t size)
{
    int ret = 0;
    asm volatile(
        "push %%ebx; movl %2,%%ebx; int $0x80; pop %%ebx"
        : "=a"(ret)
        : "0"(SYS_write), "r"(fd), "c"(buf), "d"(size));
    ERRNO_RET(ret, -1);
}

int close(int fd)
{
    int ret = 0;
    asm volatile(
        "push %%ebx; movl %2,%%ebx; int $0x80; pop %%ebx"
        : "=a"(ret)
        : "0"(SYS_close), "r"(fd));
    ERRNO_RET(ret, -1);
}

//int access(const char *, int);
//unsigned alarm(unsigned);
//int chdir(const char *);
//int chown(const char *, uid_t, gid_t);
//int dup(int);
//int dup2(int, int);
//int execl(const char *, const char *, ...);
//int execle(const char *, const char *, ...);
//int execlp(const char *, const char *, ...);
//int execv(const char *, char *const[]);

int execve(const char *file, const char **argv, const char **envp)
{
    int ret = 0;
    asm volatile(
        "push %%ebx; movl %2,%%ebx; int $0x80; pop %%ebx"
        : "=a"(ret)
        : "0"(SYS_execve), "r"(file), "c"(argv), "d"(envp));
    ERRNO_RET(ret, -1);
}

//int execvp(const char *, char *const[]);
//int faccessat(int, const char *, int, int);
//int fchdir(int);
//int fchown(int, uid_t, gid_t);
//int fchownat(int, const char *, uid_t, gid_t, int);
//int fexecve(int, char *const[], char *const[]);

pid_t fork(void)
{
    pid_t ret = 0;

    asm volatile(
        "int $0x80"
        : "=a"(ret)
        : "0"(SYS_fork));

    ERRNO_RET(ret, -1);
}

int truncate(const char *path, off_t length)
{
    int ret = 0;
    asm volatile(
        "push %%ebx; movl %2,%%ebx; int $0x80; pop %%ebx"
        : "=a"(ret)
        : "0"(SYS_truncate), "r"(path), "c"(length));
    ERRNO_RET(ret, -1);
}

int ftruncate(int fd, off_t length)
{
    int ret = 0;
    asm volatile(
        "push %%ebx; movl %2,%%ebx; int $0x80; pop %%ebx"
        : "=a"(ret)
        : "0"(SYS_ftruncate), "r"(fd), "c"(length));
    ERRNO_RET(ret, -1);
}

//long fpathconf(int, int);
//char *getcwd(char *, size_t);
//gid_t getegid(void);
//uid_t geteuid(void);
//gid_t getgid(void);
//int getgroups(int, gid_t[]);
//int gethostname(char *, size_t);
//char *getlogin(void);
//int getlogin_r(char *, size_t);
//int getopt(int, char *const[], const char *);
//pid_t getpgid(pid_t);
//pid_t getpgrp(void);
int gettid(void)
{
    pid_t ret = 0;

    asm volatile(
        "int $0x80"
        : "=a"(ret)
        : "0"(SYS_gettid));
    return ret;
}

pid_t getpid(void)
{
    pid_t ret = 0;

    asm volatile(
        "int $0x80"
        : "=a"(ret)
        : "0"(SYS_getpid));
    return ret;
}

pid_t getppid(void)
{
    pid_t ret = 0;

    asm volatile(
        "int $0x80"
        : "=a"(ret)
        : "0"(SYS_getppid));
    return ret;
}
//pid_t getsid(pid_t);
//uid_t getuid(void);
//int isatty(int);
//int lchown(const char *, uid_t, gid_t);
//int link(const char *, const char *);
//int linkat(int, const char *, int, const char *, int);

off_t lseek(int fd, off_t offset, int whence)
{
    int ret = 0;
    asm volatile(
        "push %%ebx; movl %2,%%ebx; int $0x80; pop %%ebx"
        : "=a"(ret)
        : "0"(SYS_lseek), "r"(fd), "c"(offset), "d"(whence));
    ERRNO_RET(ret, -1);
}
//long pathconf(const char *, int);
//int pause(void);
//int pipe(int[2]);
//ssize_t pread(int, void *, size_t, off_t);
//ssize_t pwrite(int, const void *, size_t, off_t);
//ssize_t readlink(const char *, char *, size_t);
//ssize_t readlinkat(int, const char *, char *, size_t);
//int rmdir(const char *);
//int setegid(gid_t);
//int seteuid(uid_t);
//int setgid(gid_t);
//int setpgid(pid_t, pid_t);
//pid_t setsid(void);
//int setuid(uid_t);
//unsigned sleep(unsigned);
//int symlink(const char *, const char *);
//int symlinkat(const char *, int, const char *);
//pid_t tcgetpgrp(int);
//int tcsetpgrp(int, pid_t);
//int truncate(const char *, off_t);
//char *ttyname(int);
//int ttyname_r(int, char *, size_t);
//int unlink(const char *);
//int unlinkat(int, const char *, int);
