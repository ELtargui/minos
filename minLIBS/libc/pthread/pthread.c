#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <mink/syscall.h>
#include <sys/mman.h>

#define THREAD_STACKSIZE 0x4000

typedef struct pthread_desciptor
{
    pthread_t tid;
    void *(*start_routine)(void *);
    void *arg;
    pthread_attr_t attr;
    struct pthread_desciptor *next;
} pthread_desciptor_t;

pthread_desciptor_t *pthread_descriptors_list = NULL;

int pthread_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void))
{
    /** TODO: fork should lock like this:
    int fork()
    {
        prepare();
        int id = sys_fork();
        if (!id)
        {
            child();
            return id;
        }
        parent();
        return id;
    }
    */
    return -1;
}

int pthread_cancel(pthread_t thread)
{
    return -1;
}

void *pthread_start(void *arg)
{
    // printf("thread started!\n");

    pthread_desciptor_t *pthd = arg;

    void *ret = pthd->start_routine(pthd->arg);

    pthread_exit(ret);
    return NULL;
}

int thread_create(uint32_t funp, uint32_t argp, uint32_t stack, uint32_t stacksize)
{
    int ret = 0;
    asm volatile(
        "push %%ebx; movl %2,%%ebx; int $0x80; pop %%ebx"
        : "=a"(ret)
        : "0"(SYS_thread), "r"(funp), "c"(argp), "d"(stack), "S"(stacksize));
    ERRNO_RET(ret, -1);
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg)
{
    pthread_desciptor_t *pthd = calloc(1, sizeof(pthread_desciptor_t));
    assert(pthd);
    pthd->start_routine = start_routine;
    pthd->arg = arg;
    if (attr)
    {
        pthd->attr = *attr;
    }
    else
    {
        pthread_attr_init(&pthd->attr);
    }

    void *addr = mmap(NULL, THREAD_STACKSIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0);
    if (addr == MAP_FAILED)
    {
        printf("mmap failed to alloc stack %s\n", strerror(errno));
        free(pthd);
        return -1;
    }

    // printf("stack at %p\n", addr);

    int e = thread_create((uint32_t)pthread_start, (uint32_t)pthd, (uint32_t)addr, THREAD_STACKSIZE);
    if (e < 0)
    {
        printf("failed to create thread (%s)\n", strerror(errno));
        free(pthd);
        return -1;
    }

    pthd->next = pthread_descriptors_list;
    pthread_descriptors_list = pthd;
    *thread = (pthread_t)e;
    return 0;
}

int pthread_equal(pthread_t t1, pthread_t t2)
{
    return t1 == t2;
}

void pthread_exit(void *ret_val)
{
    int ret = 0;
    asm volatile(
        "push %%ebx; movl %2,%%ebx; int $0x80; pop %%ebx"
        : "=a"(ret)
        : "0"(SYS_thread_exit), "r"(ret_val));
    assert(0 && "__builtin_unreachable");
    __builtin_unreachable();
}

int pthread_detach(pthread_t thread)
{
    int ret = 0;
    asm volatile(
        "push %%ebx; movl %2,%%ebx; int $0x80; pop %%ebx"
        : "=a"(ret)
        : "0"(SYS_thread_detach), "r"(thread));
    ERRNO_RET(ret, -1);
}

int pthread_join(pthread_t thread, void **ret_val)
{
    int ret = 0;
    asm volatile(
        "push %%ebx; movl %2,%%ebx; int $0x80; pop %%ebx"
        : "=a"(ret)
        : "0"(SYS_thread_join), "r"(thread), "c"(ret_val));
    ERRNO_RET(ret, -1);
}

int pthread_once(pthread_once_t *once, void (*func)(void))
{
    return -1;
}

pthread_t pthread_self(void)
{
    pid_t ret = 0;

    asm volatile(
        "int $0x80"
        : "=a"(ret)
        : "0"(SYS_gettid));
    return ret;
}
//int pthread_key_create(pthread_key_t *, void (*)(void *));
//int pthread_key_delete(pthread_key_t);

//int pthread_getconcurrency(void);
//int pthread_getcpuclockid(pthread_t, clockid_t *);
//int pthread_getschedparam(pthread_t, int *, struct sched_param *);
//void *pthread_getspecific(pthread_key_t);

//int pthread_setcancelstate(int, int *);
//int pthread_setcanceltype(int, int *);
//int pthread_setconcurrency(int);
//int pthread_setschedparam(pthread_t, int, const struct sched_param *);
//int pthread_setschedprio(pthread_t, int);
//int pthread_setspecific(pthread_key_t, const void *);
//void pthread_testcancel(void);
