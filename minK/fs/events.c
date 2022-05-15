#include <mink/types.h>
#include <mink/debug.h>
#include <mink/string.h>
#include <mink/mm.h>
#include <mink/fs.h>
#include <mink/fsevent.h>
#include <mink/process.h>
#include <errno.h>

typedef struct fseventnode
{
    fsnode_t *node;
    thread_t *thread;
    int events;
    int pending;
} fseventnode_t;

static list_t poll_nodes = {NULL, NULL, 0, NULL};
// static list_t poll_queue = {NULL, NULL, 0};
void fsevents_add(thread_t *thread, fsnode_t *node, int events)
{
    fseventnode_t *en = malloc(sizeof(fseventnode_t));
    en->thread = thread;
    en->node = node;
    en->events = events;
    en->pending = 0;
    list_append(&poll_nodes, en);
}

void fsevents_clean(thread_t *thread)
{
    interrupt_disable();
    list_node_t *node = poll_nodes.head;
    while (node)
    {
        list_node_t *next = node->next;
        fseventnode_t *en = node->value;
        if (en->thread == thread)
        {
            list_remove_node(&poll_nodes, node);
            free(node);
            free(en);
        }
        node = next;
    }
    interrupt_resume();
}

void fsnode_event(fsnode_t *node, int events)
{
    foreach ((&poll_nodes), n)
    {
        fseventnode_t *en = n->value;
        if (en->node == node)
        {
            if (en->pending)
            {
                return;
            }

            if (events & en->events)
            {
                en->pending++;
                sched_run(en->thread);
            }
        }
    }

    return;
}

int poll(pollfd_t *fds, int nfd, int timeout)
{
    int cnt = 0;
    process_t *process = current_process();
    assert(process);

    for (int i = 0; i < nfd; i++)
    {
        fds[i].revents = 0;
        filedescriptor_t *f = fd_get(process, fds[i].fd);

        if (!f)
        {
            cnt++;
            fds[i].revents = POLLNVAL;
            continue;
        }

        int e = fsnode_poll(f->node, fds[i].events);
        if (e == -1)
        {
            cnt++;
            fds[i].revents = POLLERR;
        }

        else if (e)
        {
            cnt++;
            fds[i].revents = e;
        }
    }

    if (cnt || (timeout == 0))
    {
        return cnt;
    }

    interrupt_disable();
    for (int i = 0; i < nfd; i++)
    {
        if (!fds[i].revents)
        {
            filedescriptor_t *f = fd_get(process, fds[i].fd);

            assert(f);
            fsevents_add(current_thread(), f->node, fds[i].events);
        }
    }

    interrupt_resume();

    int interrupted = 0;
    if (timeout > 0)
    {
        //timeout / 1000, (timeout % 1000) * 1000000
        timespec_t req;
        req.tv_sec = timeout / 1000;
        req.tv_nsec = (timeout % 1000) * 1000000;

        interrupted = nanosleep(&req, NULL);
    }
    else
    {
        interrupted = sched_block_on(NULL, THREAD_BLOCKED);
    }

    fsevents_clean(current_thread());
    if (interrupted)
    {
        return -EINTR;
    }

    //weakup / timeout
    for (int i = 0; i < nfd; i++)
    {
        filedescriptor_t *f = fd_get(process, fds[i].fd);
        int e = fsnode_poll(f->node, fds[i].events);
        if (e == -1)
        {
            cnt++;
            fds[i].revents = POLLERR;
        }
        else if (e)
        {
            cnt++;
            fds[i].revents = e;
        }
    }

    return cnt;
}
