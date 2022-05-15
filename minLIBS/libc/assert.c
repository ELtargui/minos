#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <min/debug.h>

void assert_failed(const char *file, int line, const char *func, const char *statement)
{
    fprintf(stderr, "assertion failed: [%s:%d] %s() => %s\n", file, line, func, statement);
    fflush(stderr);

    stack_trace(20);
    abort();
}
