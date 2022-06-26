#include <sys/types.h>
#include <mink/syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

FILE *stdin, *stdout, *stderr;
static FILE *libc_files_list = NULL;

extern void _init();
extern void _fini();
FILE *libc_open_files_list()
{
	return libc_files_list;
}
void libc_add_file(FILE *fp)
{
	fp->prev = NULL;
	fp->next = libc_files_list;

	if (libc_files_list)
		libc_files_list->prev = fp;
	libc_files_list = fp;
}

void libc_remove_file(FILE *fp)
{
	if (fp->prev)
		fp->prev->next = fp->next;
	if (fp->next)
		fp->next->prev = fp->prev;
	if (fp == libc_files_list)
		libc_files_list = fp->next;
}

void libc_exit(int status)
{
	_fini();
	int ret = 0;
	asm volatile(
		"push %%ebx; movl %2,%%ebx; int $0x80; pop %%ebx"
		: "=a"(ret)
		: "0"(SYS_exit), "r"((int)(status)));
	__builtin_unreachable();
}

extern void malloc_init();

void init_libc(int (*main)(int, char **), int argc, char *argv[])
{
	_init();
	malloc_init();
	stdin = fdopen(0, "r");
	stdin->flags |= F_FLAG_R;
	stdout = fdopen(1, "w");
	stdout->flags |= F_FLAG_W;
	stderr = fdopen(2, "w");
	stderr->flags |= F_FLAG_W;
	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IOLBF, BUFSIZ);
	setvbuf(stderr, NULL, _IOLBF, BUFSIZ);
	libc_exit(main(argc, argv));
}
