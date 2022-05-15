#include <pthread.h>

//int pthread_attr_destroy(pthread_attr_t *);
//int pthread_attr_getdetachstate(const pthread_attr_t *, int *);
//int pthread_attr_getguardsize(const pthread_attr_t *, size_t *);
//int pthread_attr_getinheritsched(const pthread_attr_t *, int *);
//int pthread_attr_getschedparam(const pthread_attr_t *, struct sched_param *);
//int pthread_attr_getschedpolicy(const pthread_attr_t *, int *);
//int pthread_attr_getscope(const pthread_attr_t *, int *);
//int pthread_attr_getstack(const pthread_attr_t *, void **, size_t *);
//int pthread_attr_getstacksize(const pthread_attr_t *, size_t *);
int pthread_attr_init(pthread_attr_t *attr)
{
    (void)attr;
    return 0;
}
//int pthread_attr_setdetachstate(pthread_attr_t *, int);
//int pthread_attr_setguardsize(pthread_attr_t *, size_t);
//int pthread_attr_setinheritsched(pthread_attr_t *, int);
//int pthread_attr_setschedparam(pthread_attr_t *, const struct sched_param *);
//int pthread_attr_setschedpolicy(pthread_attr_t *, int);
//int pthread_attr_setscope(pthread_attr_t *, int);
//int pthread_attr_setstack(pthread_attr_t *, void *, size_t);
//int pthread_attr_setstacksize(pthread_attr_t *, size_t);