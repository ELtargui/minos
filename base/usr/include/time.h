#pragma once

#include <sys/types.h>
#include <stddef.h>

typedef struct tm
{
    int tm_sec;   //Seconds [0,60].
    int tm_min;   //Minutes [0,59].
    int tm_hour;  //Hour [0,23].
    int tm_mday;  //Day of month [1,31].
    int tm_mon;   //Month of year [0,11].
    int tm_year;  //Years since 1900.
    int tm_wday;  //Day of week [0,6] (Sunday =0).
    int tm_yday;  //Day of year [0,365].
    int tm_isdst; //Daylight Savings flag.
} tm_t;

struct timespec
{
    time_t tv_sec; //Seconds.
    long tv_nsec;  //Nanoseconds.
};

struct itimerspec
{
    struct timespec it_interval; //Timer period.
    struct timespec it_value;    //Timer expiration
};

int nanosleep(const struct timespec *req, struct timespec *rem);