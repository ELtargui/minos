#pragma once
#include <stddef.h>
//#include <sys/wait.h>

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0
//RAND_MAX Maximum value returned by rand(); at least 32767.

typedef struct
{
    int quot;
    int rem;
} div_t;

typedef struct
{
    long int quot;
    long int rem;
} ldiv_t;
//lldiv_t    //Structure type returned by the lldiv() function.

void _Exit(int);
void abort(void);
//int abs(int);
int atexit(void (*)(void));
//double atof(const char *);
int atoi(const char *);
//long atol(const char *);
//long long atoll(const char *);
//void *bsearch(const void *, const void *, size_t, size_t, int (*)(const void *, const void *));
void __attribute__((malloc)) * calloc(size_t, size_t);
void __attribute__((malloc)) * valloc(size_t);
//div_t div(int, int);
//double drand48(void);
//double erand48(unsigned short[3]);
void exit(int);
void free(void *);
//int getsubopt(char **, char *const *, char **);
//int grantpt(int);
//char *initstate(unsigned, char *, size_t);
//long jrand48(unsigned short[3]);
//char *l64a(long);
//long labs(long);
//void lcong48(unsigned short[7]);
//ldiv_t ldiv(long, long);
//long long llabs(long long);
//lldiv_t lldiv(long long, long long);
//long lrand48(void);

void __attribute__((malloc)) * malloc(size_t);
//int mblen(const char *, size_t);
//size_t mbstowcs(wchar_t *, const char *, size_t);
//int mbtowc(wchar_t *, const char *, size_t);
//char *mkdtemp(char *);
//int mkstemp(char *);
//long mrand48(void);
//long nrand48(unsigned short[3]);
//int posix_memalign(void **, size_t, size_t);
//int posix_openpt(int);
//char *ptsname(int);

void qsort(void *ptr, size_t num_elements, size_t element_size, int (*compare)(const void *, const void *));
//int rand(void);
//int rand_r(unsigned *);
//long random(void);

void __attribute__((malloc)) * realloc(void *, size_t);
//char *realpath(const char *, char *);
//unsigned short *seed48(unsigned short[3]);

int putenv(char *string);
char *getenv(const char *name);
int setenv(const char *envname, const char *envval, int overwrite);
int unsetenv(const char *name);

//void setkey(const char *);
//char *setstate(char *);
//void srand(unsigned);
//void srand48(long);
//void srandom(unsigned);

double strtod(const char *nptr, char **endptr);

//float strtof(const char *, char **);
//long strtol(const char *, char **, int);
//long double strtold(const char *, char **);
//long long strtoll(const char *, char **, int);
//unsigned long strtoul(const char *, char **, int);
//unsigned long long strtoull(const char *, char **, int);
//int system(const char *);
//int unlockpt(int);
//size_t wcstombs(char *, const wchar_t *, size_t);
//int wctomb(char *, wchar_t);
