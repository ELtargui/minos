#pragma once

#include <mink/types.h>


typedef int (*stdout_putc_t)(void *, int);

void set_std_output(void *stream, stdout_putc_t putc);
int printf(const char *fmt, ...);
int sprintf(char *buf, const char *fmt, ...);
int sformat(stdout_putc_t putc, void *stream, const char *fmt, va_list vl);

extern void *std_outstream;
extern stdout_putc_t std_putc;
