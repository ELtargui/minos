#include <mink/types.h>
#include <mink/i386.h>
#include <mink/string.h>
#include <mink/lock.h>
#include <mink/mm.h>
#include <mink/process.h>
#include <mink/debug.h>

void lock(lock_t *lock)
{
    thread_t *thread = current_thread();
    if (!thread)
        return;

    while (__sync_lock_test_and_set(&lock->value, 1))
    {
        if (lock->thread_id == thread->id)
        {
            dbgln("error lock:%p(%s) threrad:%d", lock, lock->name, lock->thread_id);
            assert(0 && "dead lock");
        }
        sched_yield();
    }

    lock->thread_id = thread->id;
}

int trylock(lock_t *lock)
{
    thread_t *thread = current_thread();
    if (!thread)
        return 0;

    if (__sync_lock_test_and_set(&lock->value, 1))
    {
        // if (lock->thread_id == thread->id)
        // {
        //     dbgln("error lock:%p threrad:%d", lock, thread->id);
        //     assert(0 && "dead lock");
        // }

        return -1;
    }

    lock->thread_id = thread->id;
    return 0;
}

void unlock(lock_t *lock)
{
    thread_t *thread = current_thread();
    if (!thread)
        return;
    if (thread->id != lock->thread_id)
    {
        dbgln("error lock:%p owner:%d threrad:%d", lock, lock->thread_id, thread->id);
        assert(0 && "not the owner");
    }
    __sync_lock_release(&lock->value);
    lock->thread_id = -1;
}

void init_lock(lock_t *lock, const char *name)
{
    lock->value = 0;
    lock->thread_id = -1;
    lock->name = strdup(name);
}

lock_t *new_lock(const char *name)
{
    lock_t *lock = malloc(sizeof(lock_t));
    init_lock(lock, name);
    lock->allocated = 1;
    return lock;
}

void destroy_lock(lock_t *lock)
{
    if (lock->value)
        unlock(lock);

    free(lock->name);
    if (lock->allocated)
        free(lock);
}
