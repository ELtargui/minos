#include <stdio.h>
#include <unistd.h>
#include <string.h>

int fgetc(FILE *stream)
{
    flockfile(stream);
    int c = getc_unlocked(stream);
    funlockfile(stream);
    return c;
}

char *fgets(char *s, int size, FILE *stream)
{
    int e = 0;
    flockfile(stream);
    int i = 0;
    for (i = 0; i < size - 1; i++)
    {
        int c = getc_unlocked(stream);
        if (c == EOF)
            break;

        s[i] = c;
        if (c == '\n')
            break;
    }

    s[i] = 0;
    funlockfile(stream);

    if (e)
        return NULL;
    return s;
}

int getchar(void)
{
    return getc(stdin);
}

int getc_unlocked(FILE *stream)
{
    unsigned char c;
    if (stream->have_ungetc)
    {
        c = stream->unget_char;
        stream->have_ungetc = 0;
        return c;
    }

    int e = read(stream->fd, &c, 1);

    if (e == 0)
    {
        fprintf(stderr, "I dont now!");
        stream->eof = 1;
        return EOF;
    }
    else if (e != 1)
    {
        fprintf(stderr, "e:%d", e);
        stream->eof = 1;
        return EOF;
    }
    return c;
}

int getchar_unlocked(void)
{
    return getc_unlocked(stdin);
}

// ssize_t getdelim(char **, size_t *, int, FILE *);
// ssize_t getline(char **, size_t *, FILE *);