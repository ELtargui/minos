#pragma once

#include <sys/types.h>

struct timeval
{
    time_t tv_sec; /* seconds */
    long tv_usec;  /* microseconds */
};

struct itimerval
{
    struct timeval it_interval; //Timer interval.
    struct timeval it_value;    //Current value.
};

//int   gettimeofday(struct timeval *restrict, void *restrict);