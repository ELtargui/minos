#pragma once

#ifndef NDEBUG
extern void assert_failed(const char *file, int line, const char *func, const char *statement);
#define assert(statement) ((statement) ? (void)0 : assert_failed(__FILE__, __LINE__, __FUNCTION__, #statement))
#else
#define assert(statement) ((void)0)
#endif
