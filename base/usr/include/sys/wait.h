#pragma once
#include <sys/types.h>
#include <signal.h>

//waitpid

#define WCONTINUED (1 << 0) //Report status of continued child process.
#define WNOHANG (1 << 1)    //Do not hang if no status is available; return immediately.
#define WUNTRACED (1 << 3)  //Report status of stopped child process.

#define WEXITED (1 << 2)    //    Wait for processes that have exited.
#define WNOWAIT (1 << 3)    //    Keep the process whose status is returned in infop in a waitable state.
#define WSTOPPED (1 << 4)   //    Status is returned for any child that has stopped upon receipt of a signal.

#define WEXITSTATUS(status)  //Return exit status.
#define WIFCONTINUED(status) // True if child has been continued. [Option End]
#define WIFEXITED(status)    //    True if child exited normally.
#define WIFSIGNALED(status)  //    True if child exited due to uncaught signal.
#define WIFSTOPPED(status)   //    True if child is currently stopped.
#define WSTOPSIG(status)     //    Return signal number that caused process to stop.
#define WTERMSIG(status)     //    Return signal number that caused process to terminate.

typedef enum
{
    P_ALL = 0,
    P_PID,
    P_PGID
} idtype_t;

pid_t wait(int *stat_loc);
pid_t waitpid(pid_t pid, int *stat_loc, int options);
int waitid(idtype_t idtype, id_t id, siginfo_t *infop, int options);
