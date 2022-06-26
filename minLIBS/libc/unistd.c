#include <mink/syscall.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

extern void libc_exit(int status);
extern char **environ;

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

int dup(int fd)
{
    int ret = 0;
    asm volatile(
        "push %%ebx; movl %2,%%ebx; int $0x80; pop %%ebx"
        : "=a"(ret)
        : "0"(SYS_dup), "r"(fd));
    ERRNO_RET(ret, -1);
}

int dup2(int old, int new)
{
    return dup3(old, new, 0);
}

int dup3(int old, int new, int flags)
{
    int ret = 0;
    asm volatile(
        "push %%ebx; movl %2,%%ebx; int $0x80; pop %%ebx"
        : "=a"(ret)
        : "0"(SYS_dup3), "r"(old), "c"(new), "d"(flags));
    ERRNO_RET(ret, -1);
}

int execve(const char *file, const char **argv, const char **envp)
{
    int ret = 0;
    asm volatile(
        "push %%ebx; movl %2,%%ebx; int $0x80; pop %%ebx"
        : "=a"(ret)
        : "0"(SYS_execve), "r"(file), "c"(argv), "d"(envp));
    ERRNO_RET(ret, -1);
}

//int faccessat(int, const char *, int, int);
//int fchdir(int);
//int fchown(int, uid_t, gid_t);
//int fchownat(int, const char *, uid_t, gid_t, int);

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
//char *ttyname(int);
//int ttyname_r(int, char *, size_t);
//int unlink(const char *);
//int unlinkat(int, const char *, int);

int pipe(int fildes[2])
{
    return pipe2(fildes, 0);
}

int pipe2(int fildes[2], int flags)
{
    int ret = 0;
    asm volatile(
        "push %%ebx; movl %2,%%ebx; int $0x80; pop %%ebx"
        : "=a"(ret)
        : "0"(SYS_pipe2), "r"(fildes), "c"(flags));
    ERRNO_RET(ret, -1);
}

int execl(const char *path, const char *arg0, ... /*, (char *)0 */)
{
    if (!arg0)
        return execv(path, NULL);
    va_list vl;
    va_start(vl, arg0);

    int argc = 2;
    while (va_arg(vl, char *))
        argc++;
    va_end(vl);

    char *argv[argc];
    argv[0] = (char *)arg0;
    va_start(vl, arg0);
    for (int i = 1; i <= argc; i++)
    {
        argv[i] = (char *)va_arg(vl, char *);
    }
    argv[argc] = NULL;
    va_end(vl);

    return execv(path, (const char **)argv);
}

int execle(const char *path, const char *arg0, ... /*,(char *)0, char *const envp[]*/)
{
    va_list vl;
    if (!arg0)
    {
        va_start(vl, arg0);
        const char **envp = va_arg(vl, const char **);
        va_end(vl);
        return execve(path, NULL, envp);
    }
    va_start(vl, arg0);

    int argc = 2;
    while (va_arg(vl, char *))
        argc++;
    va_end(vl);

    char *argv[argc];
    argv[0] = (char *)arg0;
    va_start(vl, arg0);
    for (int i = 1; i <= argc; i++)
    {
        argv[i] = (char *)va_arg(vl, char *);
    }
    argv[argc] = NULL;
    (void)va_arg(vl, char *); //NULL ???

    const char **envp = va_arg(vl, const char **);
    va_end(vl);

    return execve(path, (const char **)argv, envp);
}

int execlp(const char *file, const char *arg0, ... /*, (char *)0 */)
{
    if (!arg0)
        return execvp(file, NULL);
    va_list vl;
    va_start(vl, arg0);

    int argc = 2;
    while (va_arg(vl, char *))
        argc++;
    va_end(vl);

    char *argv[argc];
    argv[0] = (char *)arg0;
    va_start(vl, arg0);
    for (int i = 1; i <= argc; i++)
    {
        argv[i] = (char *)va_arg(vl, char *);
    }
    argv[argc] = NULL;
    va_end(vl);

    return execvp(file, (const char **)argv);
}

int execv(const char *path, const char **argv)
{
    return execve(path, argv, (const char **)environ);
}

int execvpe(const char *filename, const char **argv, const char **envp);

int execvp(const char *file, const char **argv)
{
    return execvpe(file, argv, (const char **)environ);
}

int fexecve(int fd, const char **argv, const char **envp)
{
    ERRNO_RET(-ENOTSUP, -1);
}

int execvpe(const char *filename, const char **argv, const char **envp)
{
    if (strchr(filename, '/'))
        return execve(filename, argv, envp);

    char *path = getenv("PATH");
    if (!path)
        path = "/bin:/usr/bin";
    char part[256];

    int saved_errno = errno;

    char *p = path;
    while (p)
    {
        int i = 0;
        while (*p && *p != ':')
            part[i++] = *p++;
        if (*p == ':')
            p++;
        part[i] = 0;
        char candidate[256];
        sprintf(candidate, "s/s", part, filename);
        int rc = execve(candidate, argv, envp);
        if (rc < 0 && errno != ENOENT)
        {
            fprintf(stderr, "execvpe() failed on %s :: %s", candidate, strerror(errno));
            errno = saved_errno;
        }
    }

    errno = ENOENT;
    fprintf(stderr, "execvpe():ENOENT\n");
    return -1;
}
