#include <mink/types.h>
#include <mink/i386.h>
#include <mink/process.h>
#include <mink/debug.h>
#include <mink/mm.h>
#include <mink/string.h>
#include <mink/fs.h>
#include <mink/fsevent.h>
#include <mink/lock.h>
#include <mink/ringbuffer.h>
#include <errno.h>
#include <mink/stdio.h>

typedef struct pipe
{
    fsnode_t *rnode;
    fsnode_t *wnode;
    ringbuffer_t *buffer;
    int flags;
} pipe_t;

int pipe_close(fsnode_t *node)
{
    pipe_t *pipe = node->self;
    if (pipe->rnode == node)
        pipe->rnode = NULL;
    if (pipe->wnode == node)
        pipe->wnode = NULL;

    free(node);
    if (pipe->rnode == NULL && pipe->wnode == NULL)
    {
        free_ringbuffer(pipe->buffer);
        free(pipe);
    }
    return 0;
}

int pipe_read(fsnode_t *node, off_t off, size_t size, void *buf)
{
    pipe_t *pipe = node->self;
    if (pipe->rnode != node)
    {
        return -EIO;
    }

    if (pipe->wnode == NULL)
        return 0;
    
    return ringbuffer_read(pipe->buffer, size, buf);
}

int pipe_write(fsnode_t *node, off_t off, size_t size, void *buf)
{
    pipe_t *pipe = node->self;
    if (pipe->wnode != node)
    {
        return -EIO;
    }

    if (pipe->rnode == NULL)
    {
        // send SIGPIPE signal
        return -EPIPE;
    }

    return ringbuffer_write(pipe->buffer, size, buf);
}

static fsnode_ops_t pipe_ops = {
    .close = pipe_close,
    .read = pipe_read,
    .write = pipe_write,
};

int pipe(int fds[2], int flags)
{
    pipe_t *pipe = calloc(1, sizeof(pipe_t));
    pipe->buffer = new_ringbuffer(PAGESIZE, flags & O_NONBLOCK ? RB_NOBLOCK : 0);
    pipe->flags = flags;

    fsnode_t *rnode = calloc(1, sizeof(fsnode_t));
    fsnode_t *wnode = calloc(1, sizeof(fsnode_t));

    strcpy(rnode->name, "pipe");
    rnode->length = PAGESIZE;
    rnode->ops = &pipe_ops;
    rnode->type = FS_FIFO;
    rnode->self = pipe;

    strcpy(wnode->name, "pipe");
    wnode->length = PAGESIZE;
    wnode->ops = &pipe_ops;
    wnode->type = FS_FIFO;
    wnode->self = pipe;

    pipe->rnode = rnode;
    pipe->wnode = wnode;
    fds[0] = fd_alloc(current_process(), rnode, flags);
    fds[1] = fd_alloc(current_process(), wnode, flags);

    return 0;
}
