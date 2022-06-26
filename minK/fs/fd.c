#include <mink/types.h>
#include <mink/debug.h>
#include <mink/string.h>
#include <mink/fs.h>
#include <mink/mm.h>
#include <mink/region.h>
#include <mink/process.h>
#include <errno.h>

int fd_alloc(process_t *process, fsnode_t *node, int flags)
{
    if (process->files.count >= process->files.cap)
    {
        if (process->files.cap < 100)
        {
            int cap = process->files.cap;
            process->files.cap += 20;
            process->files.fds = realloc(process->files.fds, process->files.cap * sizeof(filedescriptor_t *));
            process->files.flags = realloc(process->files.flags, process->files.cap * sizeof(int));
            for (int i = cap; i < process->files.cap; i++)
            {
                process->files.fds[i] = NULL;
                process->files.flags[i] = 0;
            }
        }
    }

    for (int i = 0; i < process->files.cap; i++)
    {
        if (process->files.fds[i] == NULL)
        {
            filedescriptor_t *fdp = calloc(1, sizeof(filedescriptor_t));
            fdp->node = node;
            fdp->offset = 0;

            process->files.fds[i] = fdp;
            process->files.flags[i] = flags;
            process->files.count++;
            return i;
        }
    }

    return -ENFILE;
}

filedescriptor_t *fd_get(process_t *process, int fd)
{
    if (fd < 0 || fd > process->files.cap)
    {
        // dbgln("inavalide fd:%d", fd);
        return NULL;
    }

    if (process->files.fds[fd] == NULL)
    {
        // dbgln("inavalide fd:%d not opned", fd);
        return NULL;
    }

    return process->files.fds[fd];
}

int file_open(const char *filename, int flags, int mode)
{
    fsnode_t *node = fs_open(filename, 0, 0);
    if (!node)
    {
        if (flags & O_CREAT)
        {
            node = fs_open(filename, 1, mode);
            if (!node)
            {
                dbgln("file %s not created", filename);
                return -1;
            }
        }
        else
        {
            dbgln("file %s not found", filename);
            return -ENOENT;
        }
    }
    else if (flags & O_CREAT)
    {
        return -EEXIST;
    }
    fsnode_open(node, flags);
    return fd_alloc(current_process(), node, flags);
}

int file_close(process_t *process, int fd)
{
    if (fd < 0 || fd > process->files.cap)
    {
        dbgln("inavalide fd:%d", fd);
        return -EBADF;
    }

    if (process->files.fds[fd] == NULL)
    {
        // dbgln("not open fd:%d", fd);
        return -EBADF;
    }

    fsnode_close(process->files.fds[fd]->node);

    if (process->files.fds[fd]->ref <= 0)
        free(process->files.fds[fd]);
    else
        process->files.fds[fd]->ref--;
    process->files.fds[fd] = NULL;
    process->files.count--;
    return 0;
}

void file_close_all(process_t *process, int exec)
{
    (void)exec;
    dbgln("%d", process->files.count);
    for (int i = 0; i < process->files.cap; i++)
    {
        file_close(process, i);
    }
}

int file_read(process_t *process, int fd, void *buf, size_t size)
{
    filedescriptor_t *fdp = fd_get(process, fd);
    if (!fdp)
    {
        return -EBADF;
    }

    //is readable ??
    int r = fsnode_read(fdp->node, fdp->offset, size, buf);
    if (r < 0)
    {
        return r;
    }

    fdp->offset += r;
    return r;
}

int file_write(process_t *process, int fd, void *buf, size_t size)
{
    filedescriptor_t *fdp = fd_get(process, fd);
    if (!fdp)
    {
        return -EBADF;
    }

    //is writable ??
    int w = fsnode_write(fdp->node, fdp->offset, size, buf);
    if (w < 0)
    {
        return w;
    }

    fdp->offset += w;
    return w;
}

int file_ioctl(int fd, int cmd, void *a, void *b)
{
    filedescriptor_t *fdp = fd_get(current_process(), fd);
    if (!fdp)
    {
        return -EBADF;
    }

    return fsnode_ioctl(fdp->node, cmd, a, b);
}

int file_truncate(const char *name, off_t length)
{
    TODO();
    fsnode_t *node = fs_open(name, 0, 0);
    if (!node)
        return -ENOENT;
    return fsnode_truncate(node, length);
}

int file_fdtruncate(int fd, off_t length)
{
    filedescriptor_t *fdp = fd_get(current_process(), fd);
    if (!fdp)
    {
        return -EBADF;
    }

    return fsnode_truncate(fdp->node, length);
}

int file_map(int fd, region_t *region, off_t offset, size_t size)
{
    filedescriptor_t *desciptor = fd_get(current_process(), fd);
    assert(desciptor);
    fsnode_t *node = desciptor->node;

    fsnode_ops_t *fops = node->ops;
    if (fops->map)
    {
        return fops->map(node, region, offset, size);
    }
    else
    {
        if (node->type == FS_REG)
        {
            process_t *process = current_process();
            assert(size <= region->size);
            if (offset > node->length)
            {
                assert(0 && "EOF");
                return -EINVAL;
            }

            size_t sz = node->length - offset;
            if (size > sz)
                size = sz;

            if (size % PAGESIZE)
            {
                size &= PAGEMASK;
                size += PAGESIZE;
            }

            for (uintptr_t i = region->base; i < region->base + size; i += PAGESIZE)
            {
                if (!vm_alloc_page(make_vmpage(process->vmdir, i), 1, region->writable))
                {
                    return -ENOMEM;
                }
            }
            inavalidate_region(region->base, size);
            int ret = fsnode_read(node, offset, size, (void *)region->base);
            if (ret < 0)
            {
                ERROR(read error);
            }

            if ((size_t)ret < size)
            {
                void *p = (void *)(region->base + ret);
                memset(p, 0, size - ret);
            }

            return 0;
        }
    }

    return -ENOSTR;
}

int file_dup(int old)
{
    filedescriptor_t *fdp = fd_get(current_process(), old);
    if (!fdp)
    {
        return -EBADF;
    }

    process_t *process = current_process();

    if (process->files.count >= process->files.cap)
    {
        if (process->files.cap < 100)
        {
            int cap = process->files.cap;
            process->files.cap += 20;
            process->files.fds = realloc(process->files.fds, process->files.cap * sizeof(filedescriptor_t *));
            process->files.flags = realloc(process->files.flags, process->files.cap * sizeof(int));
            for (int i = cap; i < process->files.cap; i++)
            {
                process->files.fds[i] = NULL;
            }
        }
    }

    for (int i = 0; i < process->files.cap; i++)
    {
        if (process->files.fds[i] == NULL)
        {
            process->files.fds[i] = fdp;
            process->files.flags[i] = process->files.flags[old];
            process->files.flags[i] &= ~O_CLOEXEC;
            fdp->ref++;
            return i;
        }
    }

    return -ENFILE;
}

int file_dup3(int old, int new, int flags)
{
    filedescriptor_t *oldfdp = fd_get(current_process(), old);
    if (!oldfdp)
    {
        return -EBADF;
    }

    if (old == new)
        return new;

    filedescriptor_t *newfdp = fd_get(current_process(), new);
    if (new >= current_process()->files.cap)
    {
        return -EBADF;
    }
    if (newfdp)
    {
        file_close(current_process(), new);
    }

    current_process()->files.fds[new] = oldfdp;
    oldfdp->ref++;
    current_process()->files.flags[new] = current_process()->files.flags[old] | flags;
    return new;
}

filedescriptor_t *file_clone(process_t *process, int fd)
{
    if (process->files.fds[fd] == NULL)
        return NULL;
    filedescriptor_t *fdp = calloc(1, sizeof(filedescriptor_t));
    fdp->node = process->files.fds[fd]->node;
    fdp->offset = process->files.fds[fd]->offset;
    fsnode_open(fdp->node, process->files.flags[fd]);

    return fdp;
}
