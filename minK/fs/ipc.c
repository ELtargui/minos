#include <mink/types.h>
#include <mink/i386.h>
#include <mink/process.h>
#include <mink/debug.h>
#include <mink/mm.h>
#include <mink/string.h>
#include <mink/fs.h>
#include <mink/ringbuffer.h>
#include <errno.h>

#define MSG_MAGIC 0xc001c0de

typedef struct ipc_msg
{
    uint32_t magic;
    int event;
    uint32_t id;
    int size;
    char data[];
} ipc_msg_t;

typedef struct ipc
{
    char *name;
    ringbuffer_t *rb;
    struct ipc *server;
    list_t *clients_list;
} ipc_t;

fsnode_t *ipcd;
list_t *ipcd_list;

// ipc/web ===  server
// ipc/win ===  server
// ipc/web.client ===  client
// ipc/win.app23 ===  client

int server_read(fsnode_t *node, off_t off, size_t size, void *buf)
{
    (void)off;

    ipc_t *ipc = node->self;
    ipc_msg_t *msg = (ipc_msg_t *)buf;
    int sz = ringbuffer_read(ipc->rb, sizeof(ipc_msg_t), buf);
    if (msg->size && size >= msg->size + sizeof(ipc_msg_t))
    {
        sz += msg->size;
        if (ringbuffer_read(ipc->rb, msg->size, buf + sizeof(ipc_msg_t)) != msg->size)
        {
            return -ENODATA;
        }
    }
    return sz;
}

int server_write(fsnode_t *node, off_t off, size_t size, void *buf)
{
    (void)off;

    ipc_t *server = node->self;

    ipc_msg_t *msg = buf;
    assert(msg->magic == MSG_MAGIC);
    assert(msg->size + sizeof(ipc_msg_t) == size);

    ipc_t *ipc = NULL;
    foreach (server->clients_list, n)
    {
        ipc_t *c = n->value;
        if (msg->id == (uint32_t)c)
        {
            ipc = c;
            break;
        }
    }
    if (!ipc)
    {
        dbgln("inavalide ipc client id:%p", msg->id);
        return -1;
    }
    return ringbuffer_write(ipc->rb, size, buf);
}

int client_close(fsnode_t *node)
{
    (void)node;
    TODO(close);
    return 0;
}

int client_read(fsnode_t *node, off_t off, size_t size, void *buf)
{
    (void)off;
    ipc_t *ipc = node->self;
    ipc_msg_t *msg = (ipc_msg_t *)buf;
    int sz = ringbuffer_read(ipc->rb, sizeof(ipc_msg_t), buf);
    if (msg->size && size >= msg->size + sizeof(ipc_msg_t))
    {
        sz += msg->size;
        if (ringbuffer_read(ipc->rb, msg->size, buf + sizeof(ipc_msg_t)) != msg->size)
        {
            dbgln("read error");
            return -ENODATA;
        }
    }
    return sz;
}

int client_write(fsnode_t *node, off_t off, size_t size, void *buf)
{
    (void)off;
    ipc_t *ipc = node->self;
    ipc_t *server = ipc->server;
    ipc_msg_t *msg = buf;
    msg->id = (uint32_t)ipc;
    assert(msg->magic == MSG_MAGIC);
    assert(msg->size + sizeof(ipc_msg_t) == size);

    return ringbuffer_write(server->rb, size, buf);
}

int ipc_poll(fsnode_t *node, int events)
{
    ipc_t *ipc = node->self;
    if (events & POLLIN)
    {
        if (rb_size_toread(ipc->rb) > 0)
            return POLLIN;
    }

    if (events & POLLOUT)
    {
        if (rb_size_towrite(ipc->rb) > 0)
            return POLLOUT;
    }

    return 0;
}

static fsnode_ops_t server_ops =
    {
        .read = server_read,
        .write = server_write,
        .poll = ipc_poll,
};

static fsnode_ops_t client_ops =
    {
        .close = client_close,
        .read = client_read,
        .write = client_write,
        .poll = ipc_poll,
};

fsnode_t *ipc_find(fsnode_t *node, const char *name)
{
    (void)node;
    foreach (ipcd_list, n)
    {
        fsnode_t *fn = n->value;
        if (!strcmp(fn->name, name))
            return fn;
    }
    return NULL;
}

fsnode_t *ipc_create(fsnode_t *node, const char *name, int mode)
{
    (void)node;
    (void)mode;
    char server_name[128];
    char client_name[128];
    int i = 0;
    int is_client = 0;
    while (name[i])
    {
        if (name[i] == '.')
        {
            is_client = 1;
            server_name[i] = 0;
            int j = 0;
            i++;
            while (name[i])
                client_name[j++] = name[i++];
            client_name[j] = 0;
            break;
        }

        server_name[i] = name[i];
        i++;
        server_name[i] = 0;
    }

    if (is_client)
        dbgln("client [%s.%s]", server_name, client_name);
    else
        dbgln("server [%s]", server_name);

    fsnode_t *server = ipc_find(node, server_name);
    if (!is_client)
    {
        if (server)
        {
            dbgln("server exist %s", server_name);
            ERROR(exist);
        }

        server = calloc(1, sizeof(fsnode_t));
        ipc_t *server_ipc = calloc(1, sizeof(ipc_t));
        strcpy(server->name, server_name);
        server->type = FS_SOCK;
        server->ops = &server_ops;
        server->self = server_ipc;

        server_ipc->rb = new_ringbuffer(PAGESIZE, 0);
        server_ipc->rb->read_node = server;
        server_ipc->rb->write_node = server;
        server_ipc->clients_list = new_list();

        list_append(ipcd_list, server);
        return server;
    }
    else
    {
        if (!server)
        {
            dbgln("try to connect to unknown server [%s]", server_name);
            return NULL;
        }

        ipc_t *server_ipc = server->self;
        foreach (server_ipc->clients_list, n)
        {
            ipc_t *cipc = n->value;
            if (!strcmp(cipc->name, client_name))
            {
                dbgln("client %s exist", cipc->name);
                return NULL;
            }
        }

        fsnode_t *client_node = calloc(1, sizeof(fsnode_t));
        ipc_t *client_ipc = calloc(1, sizeof(ipc_t));
        client_ipc->server = server_ipc;
        client_ipc->rb = new_ringbuffer(PAGESIZE, 0);
        client_ipc->rb->read_node = client_node;
        client_ipc->rb->write_node = client_node;
        client_ipc->name = strdup(client_name);

        client_node->ops = &client_ops;
        strcpy(client_node->name, client_name);
        client_node->type = FS_FIFO;
        client_node->self = client_ipc;

        list_append(server_ipc->clients_list, client_ipc);

        return client_node;
    }
}

int ipc_unlink(fsnode_t *node, const char *name)
{
    (void)node;
    (void)name;
    TODO(unlik);
    return 0;
}

int ipc_readdir(fsnode_t *node, uint32_t index, int count, dirent_t *direntp)
{
    (void)node;
    uint32_t i = 0;
    int cnt = 0;
    foreach (ipcd_list, n)
    {
        if (i >= index)
        {
            fsnode_t *fn = n->value;
            dirent_t *d = direntp++;
            strcpy(d->d_name, fn->name);
            d->d_ino = fn->inode;
            d->d_type = fn->type;
            d->d_size = fn->length;

            cnt++;
            if (cnt == count)
                return cnt;
        }
        i++;
    }
    return cnt;
}

static fsnode_ops_t ipcd_ops = {
    .create = ipc_create,
    .unlink = ipc_unlink,
    .readdir = ipc_readdir,
    .find = ipc_find,
};

void ipc_install()
{
    ipcd = calloc(1, sizeof(fsnode_t));
    strcpy(ipcd->name, "ipc");
    ipcd->type = FS_DIR;
    ipcd_list = new_list();
    ipcd->self = ipcd_list;
    ipcd->ops = &ipcd_ops;
    vfs_bind("/dev/ipc", ipcd, 0666);
}
