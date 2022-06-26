#include <mink/types.h>
#include <mink/stdio.h>
#include <mink/i386.h>
#include <mink/debug.h>
#include <mink/fs.h>
#include <mink/string.h>
#include <mink/process.h>

typedef struct stack_frame
{
    struct stack_frame *next;
    uintptr_t eip;
} stack_frame_t;

extern int init_putc(void *stream, int c);

void dbg_print(const char *file, int line, const char *func, const char *fmt, ...)
{
    thread_t *thread = current_thread();
    if (thread)
    {
        char *name = current_process() ? current_process()->name : "-";
        printf("[%d:%s][%s:%d in %s()] : ", thread->id, name, file, line, func);
    }
    else
        printf("[%s:%d in %s()] : ", file, line, func);
    va_list vl;
    va_start(vl, fmt);
    sformat(std_putc, std_outstream, fmt, vl);
    va_end(vl);
    printf("\n");
}

int dbg_write(fsnode_t *node, off_t off, size_t size, void *buf)
{
    (void)node;
    (void)off;
    uint8_t *c = buf;
    for (size_t i = 0; i < size; i++)
        printf("%c", c[i]);
    return size;
}

static fsnode_ops_t dbg_ops = {
    .write = dbg_write,
};
static fsnode_t dbg_node;
fsnode_t *dbg_fsnode()
{
    fsnode_t *dbg = &dbg_node;
    strcpy(dbg->name, "dbg");
    dbg->type = FS_CHR;
    dbg->ops = &dbg_ops;
    return dbg;
}

void assert_failed(const char *file, int line, const char *func, const char *expression)
{
    CLI();
    thread_t *thread = current_thread();
    char *name = current_process() ? current_process()->name : NULL;
    if (thread)
        printf("\n\nAssertion failed  [(%s)%d][%s:%d in %s()] : \033[31m%s\033[m\n", name, thread->id, file, line, func, expression);
    else
        printf("\n\nAssertion failed  [%s:%d in %s()] : \033[31m%s\033[m\n", file, line, func, expression);

    stack_trace(20);
    while (1)
    {
        CLI();
        HLT();
    }
}

void stack_trace_impl(int max, void *frame)
{
    printf("\033[32m[%d] start stack trace:\033[0m\n", current_thread() ? current_thread()->id : 0);
    stack_frame_t *sf = (stack_frame_t *)frame;
    int i = 0;
    while (sf && i < max)
    {
        if (!sf)
            break;

        if (current_thread())
        {
            vmpage_t *p = get_vmpage(current_thread()->cpu.vmdir, (uintptr_t)sf);
            if (p == NULL || !p->present)
            {
                printf("@frame not present %p\n", sf);
                break;
            }
        }
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
