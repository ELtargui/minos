#pragma once

#include <mink/types.h>
#include <mink/list.h>

#define FS_NAME_LEN 64

#define O_RDONLY 0           //Open for reading only.
#define O_WRONLY (1 << 1)    //Open for writing only.
#define O_RDWR (1 << 2)      //Open for reading and writing.
#define O_CREAT (1 << 3)     //Create file if it does not exist.
#define O_APPEND (1 << 4)    //Set append mode.
#define O_TRUNC (1 << 5)     //Truncate flag.
#define O_NONBLOCK (1 << 6)  //Non-blocking mode.
#define O_NOFOLLOW (1 << 7)  //Do not follow symbolic links.
#define O_CLOEXEC (1 << 8)   //The FD_CLOEXEC flag associated with the new descriptor shall be set to close the file descriptor upon execution of an exec family function.
#define O_DIRECTORY (1 << 9) //Fail if file is a non-directory file.
#define O_SEARCH (1 << 10)   //Open directory for search only. The result is unspecified if this flag is applied to a non-directory file.
#define O_NOCTTY (1 << 11)   //Do not assign controlling terminal.
#define O_EXEC (1 << 12)     //Open for execute only (non-directory files). The result is unspecified if this flag is applied to a directory.
#define O_ACCMODE (1 << 13)  //Mask for file access modes.

#define POLLIN 1
#define POLLOUT 2
#define POLLERR 4
#define POLLNVAL 8
#define POLLHUP 16

#define FS_REG 1
#define FS_DIR 2
#define FS_CHR 4
#define FS_BLK 8
#define FS_FIFO 16
#define FS_SOCK 32
#define FS_LNK 64

#define S_IRUSR 0400 //Read permission, owner.
#define S_IWUSR 0200 //Write permission, owner.
#define S_IXUSR 0100 // Execute/search permission, owner.

#define S_IRWXU 0700 //Read, write, execute/search by owner.

#define S_IRGRP 040 //Read permission, group.
#define S_IWGRP 020 //Write permission, group.
#define S_IXGRP 010 //Execute/search permission, group.

#define S_IRWXG 070 //Read, write, execute/search by group.

#define S_IROTH 04 //Read permission, others.
#define S_IWOTH 02 //Write permission, others.
#define S_IXOTH 01 //Execute/search permission, others.

#define S_IRWXO 07 //Read, write, execute/search by others.

#define S_ISUID 04000 //Set-user-ID on execution.
#define S_ISGID 02000 //Set-group-ID on execution.

#define SEEK_CUR 0 // Seek relative to current position.
#define SEEK_SET 1 // Seek relative to start - of - file.
#define SEEK_END 2 // Seek relative to end - of - file.

struct region;
struct fsnode;
struct dirent;
struct stat;

typedef struct fsnode_ops
{
    int (*open)(struct fsnode *, int);
    int (*close)(struct fsnode *);
    int (*read)(struct fsnode *, off_t, size_t, void *);
    int (*write)(struct fsnode *, off_t, size_t, void *);
    int (*readdir)(struct fsnode *, uint32_t index, int count, struct dirent *);
    struct fsnode *(*find)(struct fsnode *, const char *);
    struct fsnode *(*create)(struct fsnode *, const char *, int);
    int (*ioctl)(struct fsnode *, int, void *, void *);
    int (*poll)(struct fsnode *, int);
    int (*unlink)(struct fsnode *, const char *);
    int (*map)(struct fsnode *, struct region *, off_t, size_t);
    int (*truncate)(struct fsnode *, off_t);
} fsnode_ops_t;

typedef struct fs
{
    char name[60];
    int (*mount)(const char *, struct fsnode *, int);
} fs_t;

struct vfsnode;

typedef struct fsnode
{
    char name[FS_NAME_LEN];
    int inode;
    int type;
    int mode;
    size_t length;
    fsnode_ops_t *ops;
    void *self;
    struct vfsnode *vfs;

} fsnode_t;

typedef struct vfsnode
{
    char *name;
    fsnode_t *node;
    list_t *childs;
    uint8_t ref;
    uint8_t flags;
    uint16_t mode;
} vfsnode_t;

typedef struct dirent
{
    char d_name[FS_NAME_LEN];
    int d_ino;
    int d_type;
    size_t d_size;
} dirent_t;

typedef struct stat
{
    int st_dev;              //Device ID of device containing file.
    int st_ino;              //File serial number.
    uint32_t st_mode;        //Mode of file (see below).
    int st_nlink;            //Number of hard links to the file.
    int st_uid;              //User ID of file.
    int st_gid;              //Group ID of file.
    int st_rdev;             //Device ID (if file is character or block special).
    off_t st_size;           //For regular files, the file size in bytes.
                             //For symbolic links, the length in bytes of the
                             //pathname contained in the symbolic link.
    struct timespec st_atim; // Last data access timestamp.
    struct timespec st_mtim; //Last data modification timestamp.
    struct timespec st_ctim; //Last file status change timestamp.

    int st_blksize; //A file system-specific preferred I/O block size
                    //for this object. In some file system types, this
                    //may vary from file to file.

    int st_blocks; //Number of blocks allocated for this object.u
} stat_t;

void vfs_register(const char *name, int (*mount)(const char *, fsnode_t *, int));
void fs_install();
vfsnode_t *vfs_get(const char *path, int create, const char **rpath);
vfsnode_t *vfs_bind(const char *path, fsnode_t *node, int mode);
fsnode_t *fs_open(const char *path, int create, int mode);
int fs_mount(const char *name, const char *device, const char *path, int flags);

int fsnode_open(fsnode_t *node, int flags);
int fsnode_close(fsnode_t *node);
int fsnode_read(fsnode_t *node, off_t offset, size_t size, void *buf);
int fsnode_write(fsnode_t *node, off_t offset, size_t size, void *buf);
fsnode_t *fsnode_find(fsnode_t *node, const char *name);
int fsnode_readdir(fsnode_t *node, uint32_t index, int count, dirent_t *dent);
int fsnode_truncate(fsnode_t *node, off_t length);
int fsnode_ioctl(fsnode_t *node, int cmd, void *a, void *b);
fsnode_t *fsnode_create(fsnode_t *node, const char *name, int mode);
int fsnode_poll(fsnode_t *node, int events);
int fsnode_stat(fsnode_t *node, stat_t *stat);
