#pragma once

#define asm __asm__
#define volatile __volatile__

typedef long unsigned int size_t;
typedef long int ssize_t;

typedef long clock_t;
typedef unsigned long useconds_t;
typedef long suseconds_t;

typedef int mode_t;
typedef int ino_t;
typedef int dev_t;
typedef int nlink_t;
typedef int uid_t;
typedef int gid_t;
typedef long off_t;
typedef unsigned long int time_t;
typedef unsigned int blkcnt_t;
typedef unsigned int blksize_t;

typedef int caddr_t;
typedef int pid_t;
typedef int id_t;

//Used to identify a thread.
typedef int pthread_t;

//Used to identify a thread attribute object.
typedef struct pthread_attr
{
    int tid;
} pthread_attr_t;
//Used to identify a barrier.
typedef struct pthread_barrier
{
    int tid;
} pthread_barrier_t;
//Used to define a barrier attributes object.
typedef struct pthread_barrierattr
{
    int tid;
} pthread_barrierattr_t;

//Used for condition variables.
typedef struct pthread_cond
{
    int tid;
} pthread_cond_t;

//Used to identify a condition attribute object.
typedef struct pthread_condattr
{
    int tid;
} pthread_condattr_t;

//Used for thread-specific data keys.
typedef struct pthread_key
{
    int tid;
} pthread_key_t;

//Used to identify a mutex attribute object.
typedef struct pthread_mutexattr
{
    int type;
} pthread_mutexattr_t;

//Used for mutexes.
typedef struct pthread_mutex
{
    pthread_t thread;
    int recursion;
    pthread_mutexattr_t attr;
    volatile int lock;
} pthread_mutex_t;

//Used for dynamic package initialization.
typedef struct pthread_once
{
    pthread_mutex_t mutex;
    int done;
} pthread_once_t;

//Used for read-write locks.
typedef struct pthread_rwlock
{
    int tid;
} pthread_rwlock_t;

//Used for read-write lock attributes.
typedef struct pthread_rwlockattr
{
    int tid;
} pthread_rwlockattr_t;

//Used to identify a spin lock.
typedef struct pthread_spinlock
{
    pthread_t tid;
    volatile int lock;
    int pshared;
} pthread_spinlock_t;
