#include <mink/types.h>
#include <mink/debug.h>
#include <mink/string.h>
#include <mink/mm.h>
#include <mink/fs.h>

typedef struct superblock
{
    uint32_t s_inodes_count;
    uint32_t s_blocks_count;
    uint32_t s_r_blocks_count;
    uint32_t s_free_blocks_count;
    uint32_t s_free_inodes_count;
    uint32_t s_first_data_block;
    uint32_t s_log_block_size;
    uint32_t s_log_frag_size;
    uint32_t s_blocks_per_group;
    uint32_t s_frags_per_group;
    uint32_t s_inodes_per_group;
    uint32_t s_mtime;
    uint32_t s_wtime;
    uint16_t s_mnt_count;
    uint16_t s_max_mnt_count;
    uint16_t s_magic;
    uint16_t s_state;
    uint16_t s_errors;
    uint16_t s_minor_rev_level;
    uint32_t s_lastcheck;
    uint32_t s_checkinterval;
    uint32_t s_creator_os;
    uint32_t s_rev_level;
    uint16_t s_def_resuid;
    uint16_t s_def_resgid;
    //-- EXT2_DYNAMIC_REV Specific --
    uint32_t s_first_ino;
    uint16_t s_inode_size;
    uint16_t s_block_group_nr;
    uint32_t s_feature_compat;
    uint32_t s_feature_incompat;
    uint32_t s_feature_ro_compat;
    uint8_t s_uuid[16];
    uint8_t s_volume_name[16];
    uint8_t s_last_mounted[64];
    uint32_t s_algo_bitmap; //- Performance Hints --
    uint8_t s_prealloc_blocks;
    uint8_t s_prealloc_dir_blocks;
    uint16_t align;
    //- Journaling Support --
    uint8_t s_journal_uuid[16];
    uint32_t s_journal_inum;
    uint32_t s_journal_dev;
    uint32_t s_last_orphan;
    //- Directory Indexing Support --
    uint8_t s_hash_seed[16];
    uint8_t s_def_hash_version;
    uint8_t pad[3]; //- reserved for future expansion
    uint32_t s_default_mount_options;
    uint32_t s_first_meta_bg;
    uint8_t u[760]; // Unused - reserved for future revisions
} ext2_superblock_t;

typedef struct block_group
{
    uint32_t bg_block_bitmap;
    uint32_t bg_inode_bitmap;
    uint32_t bg_inode_table;
    uint16_t bg_free_blocks_count;
    uint16_t bg_free_inodes_count;
    uint16_t bg_used_dirs_count;
    uint16_t bg_pad;
    uint8_t bg_reserved[12];
} ext2_block_group_t;

typedef struct ext2_inode
{
    uint16_t i_mode;
    uint16_t i_uid;
    uint32_t i_size;
    uint32_t i_atime;
    uint32_t i_ctime;
    uint32_t i_mtime;
    uint32_t i_dtime;
    uint16_t i_gid;
    uint16_t i_links_count;
    uint32_t i_blocks;
    uint32_t i_flags;
    uint32_t i_osd1;
    uint32_t i_block[15];
    uint32_t i_generation;
    uint32_t i_file_acl;
    uint32_t i_dir_acl;
    uint32_t i_faddr;
    uint8_t i_osd2[12];
} ext2_inode_t;

typedef struct ext2_dir
{
    uint32_t inode;
    uint16_t rec_len;
    uint8_t name_len;
    uint8_t file_type;
    uint8_t name[];
} ext2_dir_t;

#define EXT2_SUPER_MAGIC 0xEF53

#define EXT2_S_IFSOCK 0xC000 //socket
#define EXT2_S_IFLNK 0xA000  //symbolic link
#define EXT2_S_IFREG 0x8000  //regular file
#define EXT2_S_IFBLK 0x6000  //block device
#define EXT2_S_IFDIR 0x4000  //directory
#define EXT2_S_IFCHR 0x2000  //character device
#define EXT2_S_IFIFO 0x1000  //fifo

typedef struct ext2
{
    ext2_superblock_t *superblock;
    ext2_block_group_t *block_groups_table;
    int block_groups_count;
    int blocksize;
    int inodesize;
    fsnode_t *device;
    fsnode_t *root;
    int flags;
} ext2_t;

#define SB ext2->superblock
#define DEVICE ext2->device
#define BSIZE ext2->blocksize
#define ISIZE ext2->inodesize
#define ROOT ext2->root
#define BGT ext2->block_groups_table
#define BGC ext2->block_groups_count

static fsnode_t *ext2_fsnode(ext2_t *ext2, int ino, const char *name);

static int ext2_read_block(ext2_t *ext2, off_t id, void *buff)
{
    if (id > SB->s_blocks_count)
    {
        dbgln("inavalid block number :%d", id);
        return -1;
    }

    int e = fsnode_read(DEVICE, id * BSIZE, BSIZE, buff);

    if (e != BSIZE)
    {
        dbgln("read error");
        return -1;
    }
    return e;
}

static ext2_inode_t *ext2_get_inode(ext2_t *ext2, uint32_t ino)
{
    assert(ext2);
    if (ino > SB->s_inodes_count)
    {
        dbgln("error ino:%d not exist", ino);
        return NULL;
    }

    int g = (ino - 1) / SB->s_inodes_per_group;
    int gidx = (ino - 1) % SB->s_inodes_per_group;

    int table = BGT[g].bg_inode_table;

    int b_off = (gidx * ISIZE) / BSIZE;
    int i_off = (gidx * ISIZE) % BSIZE;

    void *tmp = malloc(BSIZE);

    ext2_read_block(ext2, table + b_off, tmp);

    ext2_inode_t *inode = malloc(ISIZE);
    memcpy(inode, (tmp + i_off), ISIZE);

    free(tmp);
    return inode;
}

static uint32_t inode_block_id(ext2_t *ext2, ext2_inode_t *inode, uint32_t ib)
{
    if (ib >= (inode->i_blocks * 512) / BSIZE)
    {
        dbgln("inavalide block");
        return 0xffffffff;
    }

    if (ib < 12)
    {
        return inode->i_block[ib];
    }

    uint32_t nptr = BSIZE / 4;
    uint32_t block = 0;
    uint32_t *tmp = malloc(BSIZE);

    if (ib < 12 + nptr)
    {
        ext2_read_block(ext2, inode->i_block[12], tmp);
        block = tmp[ib - 12];
        free(tmp);
        return block;
    }

    if (ib < 12 + nptr + nptr * nptr)
    {
        ext2_read_block(ext2, inode->i_block[13], tmp);

        int b0 = ib - (12 + nptr);
        int idx = b0 / nptr;
        block = tmp[idx];

        ext2_read_block(ext2, block, tmp);

        idx = b0 - (idx * nptr);
        block = tmp[idx];

        free(tmp);
        return block;
    }
    assert(0 && "triply indirect block");
    return 0xffffffff;
}

static int ext2_read_inode(ext2_t *ext2, ext2_inode_t *inode, int id, void *buff)
{
    uint32_t block = inode_block_id(ext2, inode, id);
    return ext2_read_block(ext2, block, buff);
}

int ext2_get_fstype(uint16_t i_mode)
{
    int type = 0;
    if ((i_mode & EXT2_S_IFSOCK) == EXT2_S_IFSOCK)
        type |= FS_SOCK;
    if ((i_mode & EXT2_S_IFLNK) == EXT2_S_IFLNK)
        type |= FS_LNK;
    if ((i_mode & EXT2_S_IFREG) == EXT2_S_IFREG)
        type |= FS_REG;
    if ((i_mode & EXT2_S_IFBLK) == EXT2_S_IFBLK)
        type |= FS_BLK;
    if ((i_mode & EXT2_S_IFDIR) == EXT2_S_IFDIR)
        type |= FS_DIR;
    if ((i_mode & EXT2_S_IFCHR) == EXT2_S_IFCHR)
        type |= FS_CHR;
    if ((i_mode & EXT2_S_IFIFO) == EXT2_S_IFIFO)
        type |= FS_FIFO;
    return type;
}

int ext2_open(fsnode_t *node, int flags)
{
    (void)node;
    (void)flags;
    return 0;
}

int ext2_close(fsnode_t *node)
{
    (void)node;
    return 0;
}

int ext2_read(fsnode_t *node, off_t offset, size_t size, void *buf)
{
    //dbgln("read :: %d %d", offset, size);
    ext2_t *ext2 = node->self;
    ext2_inode_t *inode = ext2_get_inode(ext2, node->inode);
    if (!inode)
    {
        dbgln("error");
        return -1;
    }

    if (offset > inode->i_size)
    {
        free(inode);
        dbgln("xxx off:%d size:%d", offset, inode->i_size);
        return -1;
    }

    if (offset + size > inode->i_size)
    {
        size = inode->i_size - offset;
    }

    uint32_t first_block = offset / BSIZE;
    uint32_t off = offset % BSIZE;
    int nblock = size / BSIZE;
    int r = 0;
    void *tmp = malloc(BSIZE);
    if (size % BSIZE)
        nblock++;
    for (uint32_t i = first_block; i < first_block + nblock; i++)
    {
        ext2_read_inode(ext2, inode, i, tmp);
        int sz = ((int)size - r) < BSIZE ? (int)size - r : BSIZE;
        memcpy(buf + r, tmp + off, sz);

        r += sz;
        off = 0;
    }
    free(tmp);
    free(inode);
    return r;
}

int ext2_write(fsnode_t *node, off_t offset, size_t size, void *buf)
{
    (void)node;
    (void)offset;
    (void)size;
    (void)buf;

    return -1;
}

fsnode_t *ext2_find(fsnode_t *parent, const char *name)
{
    assert(parent);
    ext2_t *ext2 = parent->self;
    //dbgln("ext2:%p sb:%p", ext2, SB);
    ext2_inode_t *inode = ext2_get_inode(ext2, parent->inode);
    if (!inode)
    {
        dbgln("error get inode");
        return NULL;
    }

    void *tmp = malloc(BSIZE);
    ext2_dir_t *dir;
    uint32_t size = 0;
    int off = BSIZE;
    uint32_t b = 0;

    int len = strlen(name);

    while (size < inode->i_size)
    {
        if (off >= BSIZE)
        {
            ext2_read_inode(ext2, inode, b++, tmp);
            off = 0;
        }

        dir = (ext2_dir_t *)(tmp + off);
        //dbgln("%s", dir->name);

        if (len == dir->name_len)
        {
            if (!strncmp(name, (const char *)dir->name, len))
            {
                fsnode_t *fsn = ext2_fsnode(ext2, dir->inode, name);
                free(tmp);
                free(inode);
                return fsn;
            }
        }
        size += dir->rec_len;
        off += dir->rec_len;
    }

    free(tmp);
    free(inode);
    return NULL;
}

int ext2_readdir(fsnode_t *node, uint32_t index, int count, dirent_t *entsp)
{
    assert(node);
    ext2_t *ext2 = node->self;
    ext2_inode_t *inode = ext2_get_inode(ext2, node->inode);
    if (!inode)
    {
        dbgln("error get inode");
        return 0;
    }

    memset(entsp, 0, sizeof(dirent_t) * count);

    void *tmp = malloc(BSIZE);
    ext2_dir_t *dir;
    uint32_t size = 0;
    int off = BSIZE; //to start read inode block 0
    uint32_t b = 0;
    uint32_t idx = 0;
    int cnt = 0;

    while (size < inode->i_size)
    {
        if (off >= BSIZE)
        {
            ext2_read_inode(ext2, inode, b++, tmp);
            off = 0;
        }

        dir = (ext2_dir_t *)(tmp + off);

        if (idx >= index)
        {
            ext2_inode_t *dinode = ext2_get_inode(ext2, dir->inode);
            dirent_t *dent = entsp++;
            memcpy(dent->d_name, dir->name, dir->name_len);
            assert(FS_NAME_LEN >= dir->name_len);
            dent->d_name[dir->name_len] = 0;
            // dbgln("dent %s %p/%p", dent->d_name, dent, entsp);
            dent->d_ino = dir->inode;
            dent->d_type = ext2_get_fstype(dinode->i_mode);
            dent->d_size = dinode->i_size;
            cnt++;
        }
        size += dir->rec_len;
        off += dir->rec_len;
        idx++;

        if (cnt >= count)
            break;
    }

    free(tmp);
    free(inode);
    return cnt;
}

static fsnode_ops_t ext2_file_ops = {
    .open = ext2_open,
    .close = ext2_close,
    .read = ext2_read,
    .write = ext2_write,
    .readdir = NULL,
    .find = NULL,
};

static fsnode_ops_t ext2_dir_ops = {
    .open = ext2_open,
    .close = ext2_close,
    .read = NULL,
    .write = NULL,
    .readdir = ext2_readdir,
    .find = ext2_find,
};

static fsnode_t *ext2_fsnode(ext2_t *ext2, int ino, const char *name)
{
    ext2_inode_t *inode = ext2_get_inode(ext2, ino);
    if (!inode)
        return NULL;
    //dbgln("[%s] len:%x", name, inode->i_size);

    fsnode_t *node = calloc(1, sizeof(fsnode_t));
    strcpy(node->name, name);
    node->inode = ino;
    node->self = ext2;
    node->length = inode->i_size;

    node->type = ext2_get_fstype(inode->i_mode);

    if (node->type == FS_DIR)
    {
        node->ops = &ext2_dir_ops;
    }
    else
    {
        node->ops = &ext2_file_ops;
    }

    free(inode);
    return node;
}

static int mount_ext2fs(const char *path, fsnode_t *dev, int flags)
{
    int error = 0;
    dbgln("path:%s dev:%p(%s) f:%x", path, dev, dev->name, flags);
    ext2_t *ext2 = calloc(1, sizeof(ext2_t));
    SB = malloc(sizeof(ext2_superblock_t));
    if (fsnode_read(dev, 1024, 1024, SB) != 1024)
    {
        dbgln("read sb error");
        error = 1;
        goto ret_error;
    }

    if (SB->s_magic != EXT2_SUPER_MAGIC)
    {
        dbgln("inavalide sb magic :: %x", SB->s_magic);
        error = 2;
        goto ret_error;
    }

    DEVICE = dev;
    BGC = SB->s_blocks_count / SB->s_blocks_per_group;
    if (SB->s_blocks_count % SB->s_blocks_per_group)
        BGC++;
    BSIZE = 1024 << SB->s_log_block_size;
    ISIZE = (SB->s_inode_size == 0) ? sizeof(ext2_inode_t) : SB->s_inode_size;
    assert(ISIZE == 128);
    dbgln("bsize:%d isize:%d", BSIZE, ISIZE);
    int bgt_offset = (BSIZE == 1024) ? 2048 : BSIZE;

    BGT = calloc(1, BGC * sizeof(ext2_block_group_t));
    int e = fsnode_read(DEVICE, bgt_offset, BGC * sizeof(ext2_block_group_t), BGT);
    if (e != (BGC * (int)sizeof(ext2_block_group_t)))
    {
        error = 3;
        goto ret_error;
    }

    dbgln("locate root");
    ROOT = ext2_fsnode(ext2, 2, "ext2fs");
    if (!ROOT)
    {
        dbgln("could'nt locate inode 2");
        error = 4;
        goto ret_error;
    }

    if (ROOT->type != FS_DIR)
    {
        dbgln("root inode '2' not a directory");
        error = 5;
        goto ret_error;
    }
    vfsnode_t *vfs = vfs_bind(path, ROOT, 0666);
    if (!vfs)
    {
        error = -1;
        goto ret_error;
    }

    ROOT->self = ext2;
    ROOT->vfs = vfs;
    dbgln("ext2fs mounted");
    return 0;

ret_error:
    dbgln("error::%d", error);
    if (error >= 5)
        free(ROOT);

    free(SB);
    return error;
}

void install_ext2fs()
{
    vfs_register("ext2fs", mount_ext2fs);
}
