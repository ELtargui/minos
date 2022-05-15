#include <mink/types.h>
#include <mink/debug.h>
#include <mink/string.h>
#include <mink/mm.h>
#include <mink/fs.h>

vfsnode_t vfsroot;
static list_t *filesystems;

void vfs_register(const char *name, int (*mount)(const char *, fsnode_t *, int))
{
    fs_t *fs = malloc(sizeof(fs_t));
    strcpy(fs->name, name);
    fs->mount = mount;

    list_append(filesystems, fs);
}

void fs_install()
{
    filesystems = new_list();

    vfsroot.name = strdup("[root]");
    vfsroot.childs = new_list();
    vfsroot.node = NULL;
    vfsroot.ref = -1;

    vfs_bind("/dev", NULL, 0666);
    vfs_bind("/tmp", NULL, 0666);
    vfs_bind("/mnt", NULL, 0666);

    dbgln("vfs installed\n");
}

int fs_mount(const char *name, const char *device, const char *path, int flags)
{
    fs_t *fs = NULL;
    foreach (filesystems, n)
    {
        fs_t *tmp = n->value;
        if (!strcmp(tmp->name, name))
        {
            fs = tmp;
            break;
        }
    }

    if (!fs)
    {
        return -1;
    }

    fsnode_t *dev = fs_open(device, False, 0);
    assert(dev);
    return fs->mount(path, dev, flags);
}

vfsnode_t *vfs_get(const char *path, int create, const char **rpath)
{
    assert(path[0] == '/');
    *rpath = NULL;
    if (path[1] == 0)
    {
        return &vfsroot;
    }

    vfsnode_t *vfs = &vfsroot;
    int index = 1, off = 1;
    char name[FS_NAME_LEN];

    while (path[index])
    {
        int i = 0;
        off = index;
        while (path[index] && path[index] != '/')
        {
            name[i++] = path[index++];
        }
        if (path[index] == '/')
            index++;
        name[i] = 0;
        vfsnode_t *child = NULL;

        if (!vfs->childs)
        {
            if (vfs->node && vfs->node->type != FS_DIR)
                return vfs;
            vfs->childs = new_list();
        }
        foreach (vfs->childs, n)
        {
            vfsnode_t *tmp = n->value;
            if (!strcmp(tmp->name, name))
            {
                child = tmp;
                break;
            }
        }

        if (!child)
        {
            if (create)
            {
                child = calloc(1, sizeof(vfsnode_t));
                child->node = NULL;
                child->ref = 1;
                child->name = strdup(name);
                child->mode = 0; //process mode?

                if (!vfs->childs)
                    vfs->childs = new_list();
                list_append(vfs->childs, child);
            }
            else
            {
                *rpath = path + off;
                return vfs;
            }
        }

        vfs = child;
    }
    rpath = NULL;
    return vfs;
}

vfsnode_t *vfs_bind(const char *path, fsnode_t *node, int mode)
{
    const char *rpath = NULL;
    vfsnode_t *vfs = vfs_get(path, 1, &rpath);
    assert(rpath == NULL);
    if (vfs->node)
    {
        assert(0 && vfs->node);
    }

    vfs->node = node;
    vfs->mode = mode;

    return vfs;
}

fsnode_t *fs_open(const char *path, int create, int mode)
{
    const char *rpath = NULL;
    vfsnode_t *vfs = vfs_get(path, False, &rpath);
    if (!rpath)
    {
        return vfs->node;
    }

    fsnode_t *node = vfs->node;
    if (!node)
    {
        dbgln("no vfs entry [%s] in [%s]", rpath, path);
        return NULL;
    }

    int index = 0;
    char name[FS_NAME_LEN];
    if (rpath[0] == '/')
        index++;
    while (rpath[index])
    {
        int i = 0;
        while (rpath[index] && rpath[index] != '/')
        {
            name[i++] = rpath[index++];
        }
        if (rpath[index] == '/')
            index++;
        name[i] = 0;

        fsnode_t *fn = fsnode_find(node, name);
        if (!fn)
        {
            if (create)
            {
                fn = fsnode_create(node, name, mode);
                if (!fn)
                {
                    return NULL;
                }
            }
            else
                return NULL;
        }

        vfsnode_t *vn = calloc(1, sizeof(vfsnode_t));
        vn->node = fn;
        vn->ref = 1;
        vn->name = strdup(name);
        vn->mode = create ? mode : 0; //process mode?

        if (!vfs->childs)
            vfs->childs = new_list();
        list_append(vfs->childs, vn);
        vfs = vn;
        node = fn;
    }

    return vfs->node;
}

int fsnode_open(fsnode_t *node, int flags)
{
    assert(node);
    if (node->ops->open)
        return node->ops->open(node, flags);
    return -1;
}

int fsnode_close(fsnode_t *node)
{
    assert(node);
    if (node->ops->close)
        return node->ops->close(node);
    return -1;
}

int fsnode_read(fsnode_t *node, off_t offset, size_t size, void *buf)
{
    assert(node);
    if (node->ops->read)
        return node->ops->read(node, offset, size, buf);
    return -1;
}

int fsnode_write(fsnode_t *node, off_t offset, size_t size, void *buf)
{
    assert(node);
    if (node->ops->write)
        return node->ops->write(node, offset, size, buf);
    return -1;
}

fsnode_t *fsnode_find(fsnode_t *node, const char *name)
{
    assert(node);
    if (node->ops->find)
        return node->ops->find(node, name);
    return NULL;
}

int fsnode_readdir(fsnode_t *node, uint32_t index, int count, dirent_t *dent)
{
    assert(node);
    if (node->ops->find)
        return node->ops->readdir(node, index, count, dent);
    return -1;
}

int fsnode_ioctl(fsnode_t *node, int cmd, void *a, void *b)
{
    assert(node);
    if (node->ops->ioctl)
        return node->ops->ioctl(node, cmd, a, b);
    return -1;
}

int fsnode_truncate(fsnode_t *node, off_t length)
{
    assert(node);
    if (node->ops->truncate)
        return node->ops->truncate(node, length);
    return -1;
}

fsnode_t *fsnode_create(fsnode_t *node, const char *name, int mode)
{
    assert(node);
    if (node->ops->create)
        return node->ops->create(node, name, mode);
    return NULL;
}

int fsnode_poll(fsnode_t *node, int events)
{
    assert(node);
    if (node->ops->poll)
        return node->ops->poll(node, events);
    return POLLERR;
}

int fsnode_stat(fsnode_t *node, stat_t *stat)
{
    assert(node);
    stat->st_ino = node->inode;
    if (node->type == FS_REG)
        stat->st_size = node->length;
    else if (node->type == FS_LNK)
        stat->st_size = strlen(node->self); //the length in bytes of the
                                            //pathname contained in the symbolic link.
    else
    {
        stat->st_size = node->length;
        dbgln("stat on inavalide file %d length:%d", node->name, node->length);
    }

    stat->st_mode = node->type << 16 | node->mode;

    return 0;
}
