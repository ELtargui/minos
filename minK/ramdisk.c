#include <mink/types.h>
#include <mink/debug.h>
#include <mink/string.h>
#include <mink/mm.h>
#include <mink/fs.h>

static int have_ramdisk = False;
static uintptr_t ramdisk_ptr = 0;
static size_t ramdisk_size = 0;

void ramdisk(uintptr_t start, size_t size)
{
    dbgln("[%p:%x]", start, size);
    ramdisk_ptr = start;
    ramdisk_size = size;
    have_ramdisk = True;
}

int ramdisk_read(fsnode_t *node, off_t offset, size_t size, void *buf)
{
    if (offset > node->length)
        return -1;
    if (offset + size > node->length)
    {
        size = node->length - offset;
    }

    memcpy(buf, (void *)(ramdisk_ptr + offset), size);
    return size;
}

static fsnode_ops_t rops = {
    .read = ramdisk_read,
};

void install_ramdisk()
{
    if (!have_ramdisk)
        return;
    fsnode_t *node = calloc(1, sizeof(fsnode_t));

    strcpy(node->name, "ramdisk");
    node->ops = &rops;
    node->type = FS_BLK;
    node->inode = ramdisk_ptr;
    node->length = ramdisk_size;

    node->vfs = vfs_bind("/dev/ramdisk", node, 0666);
}
