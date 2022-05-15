#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <mink/syscall.h>
#include <sys/mman.h>

int pthread_spin_destroy(pthread_spinlock_t *lock)
{
    lock->lock = 0;
    lock->pshared = -1;
    lock->tid = -1;
    return 0;
}

int pthread_spin_init(pthread_spinlock_t *lock, int pshared)
{
    lock->lock = 0;
    lock->pshared = pshared;
    lock->tid = 0;
    return 0;
}

int pthread_spin_lock(pthread_spinlock_t *lock)
{
    int self = pthread_self();
    while (__sync_lock_test_and_set(&lock->lock, 1))
    {
        if (lock->tid == self)
            ERRNO_RET(EDEADLK, -1);
    }

    lock->tid = self;
    return 0;
}

int pthread_spin_trylock(pthread_spinlock_t *lock)
{
    int self = pthread_self();

    if (__sync_lock_test_and_set(&lock->lock, 1))
    {
        if (lock->tid == self)
            ERRNO_RET(EDEADLK, -1);
        ERRNO_RET(EBUSY, -1);
    }
    lock->tid = self;
    return 0;
}

int pthread_spin_unlock(pthread_spinlock_t *lock)
{
    int self = pthread_self();
    if (self != lock->tid)
        ERRNO_RET(EPERM, -1);
    __sync_lock_release(&lock->lock);
    return 0;
}
