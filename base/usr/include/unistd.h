#pragma once

#include <sys/types.h>
#include <stdint.h>

#define F_OK (1 << 0) //    Test for existence of file.
#define R_OK (1 << 1) //    Test for read permission.
#define W_OK (1 << 2) //    Test for write permission.
#define X_OK (1 << 3) //    Test for execute (search) permission.

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

void _exit(int status);
ssize_t read(int fd, void *buf, size_t size);
ssize_t write(int fd, const void *buf, size_t size);
int close(int fd);

pid_t fork(void);

int execve(const char *file, const char **argv, const char **envp);
int gettid(void);
pid_t getpid(void);
pid_t getppid(void);
off_t lseek(int fd, off_t offset, int whence);

int truncate(const char *path, off_t length);
int ftruncate(int fd, off_t length);

int dup(int fd);
int dup2(int old, int new);
int dup3(int old, int new, int flags);

int pipe(int fildes[2]);
int pipe2(int fildes[2], int flags);

int execl(const char *path, const char *arg0, ... /*, (char *)0 */);
int execle(const char *path, const char *arg0, ... /*,(char *)0, char *const envp[]*/);
int execlp(const char *file, const char *arg0, ... /*, (char *)0 */);
int execv(const char *path, const char **argv);
int execvp(const char *file, const char **argv);
int fexecve(int fd, const char **argv, const char **envp);
