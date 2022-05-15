#include <stddef.h>
#include <pthread.h>
#include <sched.h>
#include <errno.h>

#define LOCKED 1
#define UNLOCKED 0

//int pthread_mutex_consistent(pthread_mutex_t *);
int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
    pthread_mutexattr_destroy(&mutex->attr);
    mutex->thread = -1;
    mutex->lock = UNLOCKED;
    return 0;
}
//int pthread_mutex_getprioceiling(const pthread_mutex_t *, int *);

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *mattr)
{

    if (mattr)
    {
        mutex->attr = *mattr;
    }
    else
    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        mutex->attr = attr;
    }

    return 0;
}

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
    while (!__sync_bool_compare_and_swap(&mutex->lock, UNLOCKED, LOCKED))
    {
        if (mutex->attr.type == PTHREAD_MUTEX_RECURSIVE &&
            mutex->thread == pthread_self())
        {
            mutex->recursion++;
            return 0;
        }
        sched_yield();
    }
    mutex->thread = pthread_self();
    mutex->recursion = 0;
    return 0;
}

int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
    if (!__sync_bool_compare_and_swap(&mutex->lock, UNLOCKED, LOCKED))
    {
        if (mutex->attr.type == PTHREAD_MUTEX_RECURSIVE &&
            mutex->thread == pthread_self())
        {
            mutex->recursion++;
            return 0;
        }
        errno = EBUSY;
        return -1;
    }
    mutex->thread = pthread_self();
    mutex->recursion = 0;
    return 0;
}
int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    if (!__sync_bool_compare_and_swap(&mutex->lock, UNLOCKED, LOCKED))
    {
        if (mutex->thread != pthread_self())
        {
            errno = EPERM;
            return -1;
        }

        if (mutex->attr.type == PTHREAD_MUTEX_RECURSIVE && mutex->recursion)
        {
            mutex->recursion--;
            return 0;
        }
        mutex->thread = 0;
        mutex->lock = UNLOCKED;
        return 0;
    }

    errno = ENOLCK;
    return -1;
}
//int pthread_mutex_setprioceiling(pthread_mutex_t *, int, int *);
//int pthread_mutex_timedlock(pthread_mutex_t *, const struct timespec *);
int pthread_mutexattr_destroy(pthread_mutexattr_t *mattr)
{
    mattr->type = -1;
    return 0;
}
//int pthread_mutexattr_getprioceiling(const pthread_mutexattr_t *, int *);
//int pthread_mutexattr_getprotocol(const pthread_mutexattr_t *, int *);
//int pthread_mutexattr_getpshared(const pthread_mutexattr_t *, int *);
//int pthread_mutexattr_getrobust(const pthread_mutexattr_t *, int *);
//int pthread_mutexattr_gettype(const pthread_mutexattr_t *, int *);
int pthread_mutexattr_init(pthread_mutexattr_t *mattr)
{
    mattr->type = PTHREAD_MUTEX_NORMAL;
    return 0;
}

//int pthread_mutexattr_setprioceiling(pthread_mutexattr_t *, int);
//int pthread_mutexattr_setprotocol(pthread_mutexattr_t *, int);
//int pthread_mutexattr_setpshared(pthread_mutexattr_t *, int);
//int pthread_mutexattr_setrobust(pthread_mutexattr_t *, int);

int pthread_mutexattr_settype(pthread_mutexattr_t *mattr, int type)
{
    mattr->type = type;
    return 0;
}
