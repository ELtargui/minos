#include <mink/types.h>
#include <mink/stdio.h>
#include <mink/i386.h>
#include <mink/debug.h>
#include <mink/mm.h>
#include <mink/region.h>
#include <mink/process.h>
#include <mink/string.h>
#include <errno.h>

typedef struct shm
{
    char *name;
    uint32_t *frames;
    int frames_count;

    int refcount;
} shm_t;

list_t *shm_list;

int shm_close(fsnode_t *node)
{
    (void)node;
    return 0;
}

int shm_read(fsnode_t *node, off_t offset, size_t size, void *buf)
{
    (void)node;
    (void)offset;
    (void)size;
    (void)buf;
    TODO(read);
    return 0;
}

int shm_write(fsnode_t *node, off_t offset, size_t size, void *buf)
{
    (void)node;
    (void)offset;
    (void)size;
    (void)buf;
    TODO(write);
    return 0;
}

int shm_map(fsnode_t *node, region_t *region, off_t offset, size_t size)
{
    (void)offset;
    (void)size;
    shm_t *shm = node->self;

    shm->refcount++;
    if (shm->frames)
    {
        int index = 0;
        for (uintptr_t i = region->base; i < region->base + region->size; i += PAGESIZE)
        {
            vmpage_t *p = make_vmpage(current_process()->vmdir, i);
            p->present = 1;
            p->rw = region->writable;
            p->user = 1;
            p->frame = shm->frames[index++] / PAGESIZE;
        }
    }
    else if (shm->frames_count)
    {
        /** FIXME: what if region size > frames count ?*/
        shm->frames = calloc(shm->frames_count, sizeof(uint32_t));
        int index = 0;
        for (uintptr_t i = region->base; i < region->base + region->size; i += PAGESIZE)
        {
            vmpage_t *p = make_vmpage(current_process()->vmdir, i);
            shm->frames[index++] = vm_alloc_page(p, 1, region->writable);
        }
    }

    return 0;
}

int shm_truncate(fsnode_t *node, off_t length)
{
    if (length % PAGESIZE)
    {
        length &= PAGEMASK;
        length += PAGESIZE;
    }

    shm_t *shm = node->self;

    if (shm->frames)
    {
        TODO(alredy exist);
    }
    else
    {
        shm->frames_count = length / PAGESIZE;
        shm->frames = NULL;
    }
    return 0;
}

static shm_t *get_shm(const char *name)
{
    foreach (shm_list, n)
    {
        shm_t *shm = n->value;
        if (!strcmp(shm->name, name))
            return shm;
    }

    return NULL;
}

static fsnode_ops_t shm_ops = {
    .close = shm_close,
    .read = shm_read,
    .write = shm_write,
    .map = shm_map,
    .truncate = shm_truncate,
};

fsnode_t *fsnode_from_shm(shm_t *shm)
{
    fsnode_t *node = calloc(1, sizeof(fsnode_t));

    node->self = shm;
    node->length = shm->frames_count * PAGESIZE;
    node->type = FS_BLK;
    strcpy(node->name, shm->name);
    node->inode = (int)shm;
    node->ops = &shm_ops;
    return node;
}

int shm_open(const char *name, int flags, int mode)
{
    (void)mode;

    shm_t *shm = get_shm(name);
    if (shm)
    {
        if (flags & O_CREAT)
        {
            return -EEXIST;
        }

        if (flags & O_TRUNC)
            TODO(O_TRUNC);
    }
    else if (flags & O_CREAT)
    {
        shm = malloc(sizeof(shm_t));
        shm->name = strdup(name);
        shm->frames_count = 0;
        shm->frames = NULL;
        list_append(shm_list, shm);
    }
    else
    {
        dbgln("[%s] not found", name);
        return -ENOENT;
    }

    shm->refcount++;
    return fd_alloc(current_process(), fsnode_from_shm(shm), flags);
}

int shm_unlink(const char *name)
{
    shm_t *shm = get_shm(name);
    if (!shm)
        return -ENOENT;

    list_remove(shm_list, shm);
    return 0;
}

void shm_install()
{
    shm_list = new_list();
}
