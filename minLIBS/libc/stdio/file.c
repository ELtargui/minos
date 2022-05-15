#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>

extern FILE *libc_open_files_list();
extern void libc_add_file(FILE *fp);
extern void libc_remove_file(FILE *fp);

int feof(FILE *fp)
{
    return fp->eof;
}

int ferror(FILE *fp)
{
    return fp->error;
}

void clearerr(FILE *fp)
{
    fp->error = 0;
    fp->eof = 0;
}

int fileno(FILE *fp)
{
    return fp->fd;
}

void flockfile(FILE *stream)
{
    pthread_mutex_lock(&stream->mutex);
}

int ftrylockfile(FILE *stream)
{
    return pthread_mutex_trylock(&stream->mutex);
}

void funlockfile(FILE *stream)
{
    pthread_mutex_unlock(&stream->mutex);
}

int fflush(FILE *stream)
{
    if (stream == NULL)
    {
        int e = 0;
        FILE *fp = libc_open_files_list();
        while (fp)
        {
            flockfile(fp);
            e |= fflush(fp);
            funlockfile(fp);
            fp = fp->next;
        }

        return e;
    }

    if (stream->buffer_type == _IONBF)
    {
        return 0;
    }
    if (stream->flags & F_FLAG_W)
    {
        if (stream->b_off)
        {
            int e = write(stream->fd, stream->buffer, stream->b_off);
            if (e < stream->b_off)
            {
                stream->b_off = 0;
                stream->eof = 1;
                return EOF;
            }
            stream->b_off = 0;
            stream->buffer[0] = 0;

            return 0;
        }
    }
    else
    {
        if (stream->have_ungetc)
        {
            stream->have_ungetc = 0;
            stream->unget_char = 0;
        }
        //discard input
    }

    return 0;
}

int setvbuf(FILE *stream, char *buf, int type, size_t size)
{
    if (type != _IONBF &&
        type != _IOLBF &&
        type != _IOFBF)
    {
        ERRNO_RET(EINVAL, -1);
    }
    stream->buffer_type = type;
    if (!size)
        size = BUFSIZ;
    if (!buf)
    {
        stream->flags |= F_FLAG_AUTOBUF;
        buf = calloc(1, size);
    }
    stream->buffer = (unsigned char *)buf;
    stream->bufsize = size;
    return 0;
}

void setbuf(FILE *stream, char *buf)
{
    if (buf)
        setvbuf(stream, buf, _IOFBF, BUFSIZ);
    else
        setvbuf(stream, buf, _IONBF, BUFSIZ);
}

static FILE *new_file(int fd, int flags, int buffer_type)
{
    FILE *fp = calloc(1, sizeof(FILE));
    if (!fp)
        return NULL;
    fp->fd = fd;
    fp->flags = flags;
    fp->buffer_type = buffer_type;

    if (buffer_type == _IONBF)
        return fp;
    if (setvbuf(fp, NULL, buffer_type, BUFSIZ) < 0)
    {
        free(fp);
        return NULL;
    }
    libc_add_file(fp);
    return fp;
}

static int parse_mode(const char *mode)
{
    const char *m = mode;
    int flags = 0;
    while (*m)
    {
        switch (*m)
        {
        case 'r':
            flags |= O_RDONLY;
            break;
        case 'w':
            flags |= O_WRONLY;
            break;
        case 'a':
            flags |= O_WRONLY | O_CREAT | O_APPEND;
            break;
        case '+':
            flags &= ~O_WRONLY;
            flags |= O_RDWR;
            break;
        case 'b':
            break;
        }
        m++;
    }

    return flags;
}

FILE *fdopen(int fd, const char *mode)
{
    int flags = parse_mode(mode);
    int ff = 0;
    if (flags & O_RDWR)
    {
        ff = F_FLAG_R | F_FLAG_W;
    }
    else if (flags & O_WRONLY)
    {
        ff = F_FLAG_R;
    }
    else
    {
        ff = F_FLAG_R;
    }
    return new_file(fd, ff, _IONBF);
}

FILE *fopen(const char *filename, const char *mode)
{
    int flags = parse_mode(mode);
    int fd = open(filename, flags, 0666);
    if(fd < 0)
    {
        return NULL;
    }
    int ff = 0;
    int buf = _IONBF;
    if (flags & O_RDWR)
    {
        ff = F_FLAG_R | F_FLAG_W;
    }
    else if (flags & O_WRONLY)
    {
        ff = F_FLAG_R;
        buf = _IOLBF;
    }
    else
    {
        ff = F_FLAG_R;
    }
    return new_file(fd, ff, buf);
}

FILE *freopen(const char *filename, const char *mode, FILE *fp)
{
    return NULL;
}

int fclose(FILE *stream)
{
    int fd = stream->fd;
    fflush(stream);
    if (stream->flags & F_FLAG_AUTOBUF)
    {
        free(stream->buffer);
    }
    libc_remove_file(stream);
    free(stream);

    return close(fd);
}
