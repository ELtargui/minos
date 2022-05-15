#pragma once

#include <stdint.h>

typedef struct inflate
{
    void *inparam;
    void *outparam;
    int (*get)(void *);
    int (*put)(void *, int);

    uint8_t *window;
    int winsize;
    int winpos;

    uint8_t bitsBuffer;
    int bitsLeft;

    int error;

    int read_size;
    int write_size;
} inflate_t;

int inflate(inflate_t *inflate);
