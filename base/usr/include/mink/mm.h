#pragma once

#define PAGESIZE 0x1000
#define PAGEMASK 0xfffff000
#define MB 0x100000

#define PROT_NONE 0       //Page cannot be accessed.
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

void *mm_alloc(size_t size);
uint32_t alloc_frame(int cnt);

void frame_set_allocated(uint32_t frame, int b);
uintptr_t mm_get_paddr(uintptr_t addr);

void install_mem(void *bootinfo);
void *mm_heap_alloc(size_t size);
void mm_heap_free(uintptr_t ptr, size_t size);

void * malloc(size_t size);
void * calloc(size_t nmemb, size_t size);
void * valloc(size_t size);
void * realloc(void *ptr, size_t size);
void free(void *ptr);



int shm_open(const char *name, int flags, int mode);
int shm_unlink(const char *name);
void shm_install();
