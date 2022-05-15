#include <stdio.h>
#include <unistd.h>

size_t fread(void *ptr, size_t size, size_t nitems, FILE *stream)
{
    unsigned char *buffer = (unsigned char *)ptr;

    for (size_t i = 0; i < nitems; i++)
    {
        for (size_t j = 0; j < size; j++)
        {

            int c = fgetc(stream);
            if (c == EOF)
                return i;
            buffer[i * size + j] = c;
        }
    }

    return nitems;
}