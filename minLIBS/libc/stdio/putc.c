#include <stdio.h>
#include <unistd.h>
#include <string.h>

int putchar(int c)
{
    return fputc(c, stdout);
}

int putc_unlocked(int c, FILE *stream)
{
    if (stream->buffer_type == _IONBF)
    {
        unsigned char b[1];
        b[0] = c;
        if (write(stream->fd, b, 1) != 1)
            return -1;
        return c;
    }

    if (stream->b_off >= stream->bufsize)
    {
        if (fflush(stream) == EOF)
        {
            return EOF;
        }
    }
    stream->buffer[stream->b_off++] = c;
    stream->buffer[stream->b_off] = 0;
    if ((stream->buffer_type == _IOLBF) && (c == '\n'))
    {
        if (fflush(stream) == EOF)
        {
            return EOF;
        }
    }

    return c;
}

int putchar_unlocked(int c)
{
    return putc_unlocked(c, stdout);
}

int fputc(int c, FILE *stream)
{
    flockfile(stream);
    int ret = putc_unlocked(c, stream);
    funlockfile(stream);
    return ret;
}

int puts(const char *s)
{
    int len = strlen(s);
    fwrite(s, 1, len, stdout);
    return fputc('\n', stdout);
}

int fputs(const char *s, FILE *stream)
{
    int len = strlen(s);
    int e = fwrite(s, 1, len, stream);
    if (e < 0)
        return e;
    return len;
}
