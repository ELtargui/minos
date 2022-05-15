#pragma once

typedef struct lock
{
    volatile int value;
    char *name;
    int thread_id;
    int allocated;
} lock_t;

void lock(lock_t *lock);
int trylock(lock_t *lock);
void unlock(lock_t *lock);
void init_lock(lock_t *lock, const char *name);
lock_t *new_lock(const char *name);
void destroy_lock(lock_t *lock);
