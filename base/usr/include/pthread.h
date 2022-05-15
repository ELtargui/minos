#pragma once

#include <sys/types.h>

#define PTHREAD_BARRIER_SERIAL_THREAD
#define PTHREAD_CANCEL_ASYNCHRONOUS
#define PTHREAD_CANCEL_ENABLE
#define PTHREAD_CANCEL_DEFERRED
#define PTHREAD_CANCEL_DISABLE
#define PTHREAD_CANCELED

#define PTHREAD_CREATE_DETACHED 0
#define PTHREAD_CREATE_JOINABLE 1

#define PTHREAD_INHERIT_SCHED 0
#define PTHREAD_EXPLICIT_SCHED 1

#define PTHREAD_MUTEX_DEFAULT 0
#define PTHREAD_MUTEX_ERRORCHECK 1 << 0
#define PTHREAD_MUTEX_NORMAL 0
#define PTHREAD_MUTEX_RECURSIVE 1 << 2
#define PTHREAD_MUTEX_ROBUST 1 << 3
#define PTHREAD_MUTEX_STALLED 1 << 4

#define PTHREAD_ONCE_INIT

#define PTHREAD_PRIO_INHERIT 0
#define PTHREAD_PRIO_NONE 1
#define PTHREAD_PRIO_PROTECT 2

#define PTHREAD_PROCESS_PRIVATE 0
#define PTHREAD_PROCESS_SHARED 1

#define PTHREAD_SCOPE_PROCESS 0
#define PTHREAD_SCOPE_SYSTEM 1

#define PTHREAD_COND_INITIALIZER \
    {                            \
        NULL                     \
    } //pthread_cond_t

#define PTHREAD_MUTEX_INITIALIZER \
    {                             \
        NULL                      \
    } //pthread_mutex_t

#define PTHREAD_RWLOCK_INITIALIZER \
    {                              \
        NULL                       \
    } //pthread_rwlock_t

int pthread_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void));
int pthread_cancel(pthread_t thread);
void *pthread_start(void *arg);
int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void *), void *arg);
int pthread_detach(pthread_t thread);
int pthread_equal(pthread_t t1, pthread_t t2);
void pthread_exit(void *ret_val);
int pthread_join(pthread_t thread, void **ret_val);
int pthread_once(pthread_once_t *once, void (*func)(void));
pthread_t pthread_self(void);

int pthread_spin_destroy(pthread_spinlock_t *);
int pthread_spin_init(pthread_spinlock_t *, int);
int pthread_spin_lock(pthread_spinlock_t *);
int pthread_spin_trylock(pthread_spinlock_t *);
int pthread_spin_unlock(pthread_spinlock_t *);

//int pthread_mutex_consistent(pthread_mutex_t *);
int pthread_mutex_destroy(pthread_mutex_t *mutex);
//int pthread_mutex_getprioceiling(const pthread_mutex_t *, int *);
int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *mattr);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_trylock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
//int pthread_mutex_setprioceiling(pthread_mutex_t *, int, int *);
//int pthread_mutex_timedlock(pthread_mutex_t *, const struct timespec *);
int pthread_mutexattr_destroy(pthread_mutexattr_t *mattr);
//int pthread_mutexattr_getprioceiling(const pthread_mutexattr_t *, int *);
//int pthread_mutexattr_getprotocol(const pthread_mutexattr_t *, int *);
//int pthread_mutexattr_getpshared(const pthread_mutexattr_t *, int *);
//int pthread_mutexattr_getrobust(const pthread_mutexattr_t *, int *);
//int pthread_mutexattr_gettype(const pthread_mutexattr_t *, int *);
int pthread_mutexattr_init(pthread_mutexattr_t *mattr);
//int pthread_mutexattr_setprioceiling(pthread_mutexattr_t *, int);
//int pthread_mutexattr_setprotocol(pthread_mutexattr_t *, int);
//int pthread_mutexattr_setpshared(pthread_mutexattr_t *, int);
//int pthread_mutexattr_setrobust(pthread_mutexattr_t *, int);
int pthread_mutexattr_settype(pthread_mutexattr_t *mattr, int type);

int pthread_attr_init(pthread_attr_t *attr);