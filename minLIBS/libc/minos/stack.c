#include <stdio.h>
#include <ubsan.h>
#include <unistd.h>
#include <sys/types.h>
#include <min/debug.h>

typedef struct stack_frame
{
    struct stack_frame *next;
    unsigned int eip;
} stack_frame_t;

void stack_trace_impl(int max, void *frame)
{
    printf("[%d] start stack trace:\n", gettid());
    stack_frame_t *sf = (stack_frame_t *)frame;
    int i = 0;
    while (sf && i < max)
    {
        if (!sf)
            break;

        symbole_t *s = symbole_from_address(sf->eip);
        if (s)
        {
            printf("@ %p: %s (%d)\n", sf->eip, s->name, sf->eip - s->address);
        }
        else
        {
            printf("@ %p:\n", sf->eip);
        }

        if (sf->eip < 0x1000)
            break;

        sf = sf->next;
        i++;
    }
}

void stack_trace(int maxframes)
{
    static int tracing = 0;

    if (tracing)
        return;
    tracing = 1;

    void *ebp = 0;
    asm volatile("mov %%ebp, %0"
                 : "=r"(ebp));
    stack_trace_impl(maxframes, ebp);

    tracing = 0;
}
