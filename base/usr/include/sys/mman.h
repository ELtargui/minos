#pragma once

#include <sys/types.h>


#define PROT_NONE 0  //Page cannot be accessed.
#define PROT_EXEC 1 << 0  //Page can be executed. 
#define PROT_READ 1 << 1  //Page can be read.
#define PROT_WRITE 1 << 2 //Page can be written.

#define MAP_FIXED 1 << 0     //Interpret addr exactly.
#define MAP_PRIVATE 1 << 1   //Changes are private.
#define MAP_SHARED 1 << 2    //Share changes.
#define MAP_ANONYMOUS 1 << 3 //The mapping is not backed by any file.
#define MAP_STACK 1 << 4
#define MAP_LAZY 1 << 5

#define MAP_FAILED (void *)-1

#define POSIX_MADV_DONTNEED   //The application expects that it will not access the specified range in the near future.
#define POSIX_MADV_NORMAL     //The application has no advice to give on its behavior with respect to the specified range. It is the default characteristic if no advice is given for a range of memory.
#define POSIX_MADV_RANDOM     //The application expects to access the specified range in a random order.
#define POSIX_MADV_SEQUENTIAL //The application expects to access the specified range sequentially from lower addresses to higher addresses.
#define POSIX_MADV_WILLNEED   //The application expects to access the specified range in the near future.

void *mmap(void *addr, size_t len, int prot, int flags, int fildes, off_t off);
int mprotect(void *addr, size_t len, int prot);
int munmap(void *addr, size_t len);
int posix_madvise(void *addr, size_t len, int advice);

int shm_open(const char *name, int oflag, mode_t mode);
int shm_unlink(const char *name);
