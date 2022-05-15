#pragma once
#include <sys/types.h>

#define SIGABRT 1    //    A   Process abort signal.
#define SIGALRM 2    //    T   Alarm clock.
#define SIGBUS 3     //    A   Access to an undefined portion of a memory object.
#define SIGCHLD 4    //    I   Child process terminated, stopped, or continued.
#define SIGCONT 5    //    C   Continue executing, if stopped.
#define SIGFPE 6     //    A   Erroneous arithmetic operation.
#define SIGHUP 7     //    T   Hangup.
#define SIGILL 8     //    A   Illegal instruction.
#define SIGINT 9     //    T   Terminal interrupt signal.
#define SIGKILL 10   //    T   Kill (cannot be caught or ignored).
#define SIGPIPE 11   //    T   Write on a pipe with no one to read it.
#define SIGQUIT 12   //    A   Terminal quit signal.
#define SIGSEGV 13   //    A   Invalid memory reference.
#define SIGSTOP 14   //    S   Stop executing (cannot be caught or ignored).
#define SIGTERM 15   //    T   Termination signal.
#define SIGTSTP 16   //    S   Terminal stop signal.
#define SIGTTIN 17   //    S   Background process attempting read.
#define SIGTTOU 18   //    S   Background process attempting write.
#define SIGUSR1 19   //    T   User-defined signal 1.
#define SIGUSR2 20   //    T   User-defined signal 2.
#define SIGPOLL 21   //    T   Pollable event.
#define SIGPROF 22   //    T   Profiling timer expired.
#define SIGSYS 23    //    A   Bad system call.
#define SIGTRAP 24   //    A   Trace/breakpoint trap.
#define SIGURG 25    //    I   High bandwidth data is available at a socket.
#define SIGVTALRM 26 //    T   Virtual timer expired.
#define SIGXCPU 27   //    A   CPU time limit exceeded.
#define SIGXFSZ 28   //    A    File size limit exceeded.
#define NSIG 28

#define SIG_DFL 0  //Request for default signal handling.
#define SIG_ERR -1 //Return value from signal() in case of error.
#define SIG_IGN 1  //Request that signal be ignored.
#define SIG_HOLD 2 //Request that signal be held.

typedef unsigned int sigset_t;

union sigval
{
    int sival_int;
    void *sival_ptr;
};

typedef struct siginfo
{
    int si_signo;          //  Signal number.
    int si_code;           //   Signal code.
    int si_errno;          //  If non-zero, an errno value associated with this signal
    pid_t si_pid;          //    Sending process ID.
    uid_t si_uid;          //    Real user ID of sending process.
    void *si_addr;         //   Address of faulting instruction.
    int si_status;         // Exit value or signal.
    long si_band;          //   Band event for SIGPOLL.
    union sigval si_value; //  Signal value.
} siginfo_t;

struct sigaction
{
    void (*sa_handler)(int);                        //Pointer to a signal-catching function or one of the SIG_IGN or SIG_DFL.
    sigset_t sa_mask;                               //signals to be blocked during execution
    int sa_flags;                                   //Special flags.
    void (*sa_sigaction)(int, siginfo_t *, void *); //Pointer to a signal-catching function.
};