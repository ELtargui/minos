#pragma once

#define __KERNEL__
#define __MINK__

#define NULL ((void *)0)
#define True 1
#define False 0

#define UINTMAX 0xffffffff
#define INTMAX 0x7fffffff

#define va_start(vl, arg) __builtin_va_start(vl, arg)
#define va_arg(vl, type) __builtin_va_arg(vl, type)
#define va_end(vl) __builtin_va_end(vl)
#define va_copy(d, s) __builtin_va_copy(d, s)
#define asm __asm__

#define volatile __volatile__

typedef __builtin_va_list va_list;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed long long int64_t;

typedef unsigned int size_t;
typedef unsigned long uintptr_t;

typedef int ssize_t;
typedef signed long intptr_t;

typedef unsigned long off_t;
typedef unsigned long time_t;

typedef struct timespec
{
    time_t tv_sec; //Seconds.
    long tv_nsec;  //Nanoseconds.
} timespec_t;

int to_digit(int d);
int to_xdigit(int x);
int atoi(const char *s) ;
