#include <stdio.h>
#include <unistd.h>

size_t fwrite(const void *ptr, size_t size, size_t nitems, FILE *stream)
{

    unsigned char *buffer = (unsigned char *)ptr;
    if (stream->buffer_type == _IONBF)
    {
        size_t i = 0;
        size_t w = 0;
        while (i < nitems)
        {
            int e = write(stream->fd, buffer + w, size);
            if (e < 0)
            {
                stream->error = 1;
                return w / i;
            }
            if (e == 0)
            {
                stream->eof = 1;
                return w / i;
            }

            w += e;
        }
    }

    flockfile(stream);
    for (size_t i = 0; i < nitems; i++)
    {
        for (size_t j = 0; j < size; j++)
        {
            if (putc_unlocked(buffer[i * size + j], stream) == EOF)
                return i;
        }
    }
    funlockfile(stream);


    return nitems;
}