#pragma once

#include <signal.h>

struct pollfd
{
    int fd;
    int events;
    int revents;
};

typedef int nfds_t;

#define POLLIN 1
#define POLLOUT 2
#define POLLERR 4
#define POLLNVAL 8
#define POLLHUP 16

int poll(struct pollfd *fds, nfds_t nfds, int timeout);
//int ppoll(struct pollfd *fds, nfds_t nfds, const struct timespec *tmo_p, const sigset_t *sigmask);
