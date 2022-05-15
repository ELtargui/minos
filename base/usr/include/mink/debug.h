#pragma once

typedef struct symbole
{
    char *name;
    uintptr_t address;
} symbole_t;

#define dbgln(...) dbg_print(__FILE__, __LINE__, __func__, __VA_ARGS__)
#define assert(expression) ((expression) ? (void)0 : assert_failed(__FILE__, __LINE__, __func__, #expression))
#define TODO(expression) assert(0 && "TODO" && #expression)
#define ERROR(expression) assert(0 && "ERROR" && #expression)

void dbg_print(const char *file, int line, const char *func, const char *fmt, ...);
void assert_failed(const char *file, int line, const char *func, const char *expression);

void load_symboles();
symbole_t *symbole_from_name(const char *name);
symbole_t *symbole_from_address(uint32_t addr);

void stack_trace_impl(int max, void *frame);
void stack_trace(int maxframes);


