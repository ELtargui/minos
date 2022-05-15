#pragma once

#include <mink/fs.h>
#include <mink/process.h>

typedef struct pollfd
{
    int fd;
    int events;
    int revents;
} pollfd_t;

void fsnode_event(fsnode_t *node, int events);
void fsevents_add(thread_t *thread, fsnode_t *node, int events);
void fsevents_clean(thread_t *thread);
int poll(pollfd_t *fds, int nfd, int timeout);
