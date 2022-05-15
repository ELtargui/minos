#include <stdio.h>
#include <unistd.h>
#include <assert.h>

int fseek(FILE *stream, long offset, int whence)
{
    int e = fflush(stream);
    if (e)
        return -1;
        
    off_t off = lseek(stream->fd, offset, whence);
    if (off < 0)
    {
        return -1;
    }

    return 0;
}

long ftell(FILE *stream)
{
    int e = fflush(stream);
    if (e)
        return -1;
    return lseek(stream->fd, 0, SEEK_CUR);
}

void rewind(FILE *stream)
{
    (void)fseek(stream, 0L, SEEK_SET);
}

int fgetpos(FILE *stream, fpos_t *pos)
{
    assert(pos);
    long p = ftell(stream);
    if (p == -1)
        return -1;
    *pos = (fpos_t)p;
    return 0;
}

int fsetpos(FILE *stream, const fpos_t *pos)
{
    assert(pos);
    return fseek(stream, *pos, SEEK_SET);
}
