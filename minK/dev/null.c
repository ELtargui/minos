#include <mink/types.h>
#include <mink/debug.h>
#include <mink/string.h>
#include <mink/mm.h>
#include <mink/fs.h>

int null_read(fsnode_t *node, off_t offset, size_t size, void *buf)
{
    (void)node;
    (void)offset;
    memset(buf, 0, size);
    return size;
}

int null_write(fsnode_t *node, off_t offset, size_t size, void *buf)
{
    (void)node;
    (void)offset;
    (void)buf;
    return size;
}

static fsnode_ops_t null_ops = {
    .read = null_read,
    .write = null_write,
};

void null_install()
{
    fsnode_t *node = calloc(1, sizeof(fsnode_t));

    strcpy(node->name, "null");
    node->ops = &null_ops;
    node->type = FS_CHR;
    node->inode = (int)node;
    node->length = 0;

    node->vfs = vfs_bind("/dev/null", node, 0666);
}
