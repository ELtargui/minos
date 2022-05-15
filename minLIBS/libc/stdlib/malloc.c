#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>

#define PAGESIZE 0x1000
#define PAGEMASK 0xfffff000

#define dbgln(...) dbg_print(__FILE__, __LINE__, __func__, __VA_ARGS__)

static void dbg_print(const char *file, int line, const char *func, const char *fmt, ...)
{
    dprintf(2, "[%s:%d in %s()] : ", file, line, func);
    va_list vl;
    va_start(vl, fmt);
    vdprintf(2, fmt, vl);
    dprintf(2, "\n");
    va_end(vl);
}

void *os_alloc(size_t size)
{
    void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (ptr == MAP_FAILED)
        assert(0 && "out of memory");
    memset(ptr, 0, size);

    return ptr;
}

void os_free(uintptr_t ptr, size_t size)
{
    munmap((void *)ptr, size);
}

#define BLOCK_MAGIC 0xdeadc0de
#define BIN_MAGIC 0xdeadbeef
#define HEADER_MAGIC 0xfdadc0de

struct allocator;

static pthread_mutex_t m_lock;

typedef struct malloc_bin
{
    uint32_t magic;
    struct malloc_bin *next;
} malloc_bin_t;

typedef struct malloc_block
{
    uint32_t magic;
    malloc_bin_t *bin;
    struct allocator *allocator;
    struct malloc_block *next;
} malloc_block_t;

typedef struct allocator
{
    malloc_block_t *block;
    uint32_t bin_size;
    uint32_t block_size;
    int n_block;
    int n_alloc;
    int n_free;
} allocator_t;

typedef struct block_header
{
    uint32_t magic;
    uint32_t size;
    uintptr_t *ptr;
    struct block_header *next;
} block_header_t;

static allocator_t m_malloc[] = {
    {NULL, 16, 0x1000, 0, 0, 0},
    {NULL, 32, 0x1000, 0, 0, 0},
    {NULL, 64, 0x1000, 0, 0, 0},
    {NULL, 128, 0x1000, 0, 0, 0},
    {NULL, 256, 0x1000, 0, 0, 0},
    {NULL, 512, 0x1000, 0, 0, 0},
    {NULL, 1024, 0x1000, 0, 0, 0},
    {NULL, 2040, 0x1000, 0, 0, 0},
};

block_header_t *allocated_block_header = NULL;

void malloc_init()
{
    pthread_mutex_init(&m_lock, NULL);
}

allocator_t *get_allocator(uint32_t size)
{
    for (uint32_t i = 0; i < 8; i++)
    {
        if (m_malloc[i].bin_size >= size)
            return &m_malloc[i];
    }
    return NULL;
}

void *alloc(uint32_t size)
{
    allocator_t *allocator = get_allocator(size);
    if (!allocator)
    {
        assert(size > 2040);
        if (size % PAGESIZE)
        {
            size &= PAGEMASK;
            size += PAGESIZE;
        }
        void *ptr = os_alloc(size);
        block_header_t *header = alloc(sizeof(block_header_t));
        header->magic = HEADER_MAGIC;
        header->size = size;
        header->ptr = (uintptr_t *)ptr;
        header->next = allocated_block_header;
        allocated_block_header = header;
        return ptr;
    }

    malloc_block_t *block = allocator->block;

    while (block)
    {
        if (block->magic != BLOCK_MAGIC)
        {
            dbgln("inavalide block magic(deadcode) [%p m:%p, n:%p a:%p bin:%p]", block, block->magic, block->next, block->allocator, block->bin);
        }
        assert(block->magic == BLOCK_MAGIC);
        if (block->bin)
            break;
        block = block->next;
    }

    if (!block)
    {
        block = (malloc_block_t *)os_alloc(allocator->block_size);
        block->magic = BLOCK_MAGIC;
        block->allocator = allocator;
        block->next = allocator->block;
        allocator->block = block;
        allocator->n_block++;

        uintptr_t addr = (uintptr_t)block + sizeof(malloc_block_t);
        uintptr_t end = (uintptr_t)block + allocator->block_size;
        block->bin = (malloc_bin_t *)addr;
        while (1)
        {
            malloc_bin_t *bin = (malloc_bin_t *)addr;

            addr += allocator->bin_size;

            bin->magic = BIN_MAGIC;
            bin->next = (malloc_bin_t *)addr;

            if (addr + allocator->bin_size > end)
            {
                bin->next = NULL;
                break;
            }
        }
    }

    assert(block);
    malloc_bin_t *bin = block->bin;
    assert(bin);

    if ((uintptr_t)block == 0x400F1000)
    {
        dbgln("allocator::%d b:%p n:%p m:%p\n", allocator->bin_size, bin, bin->next, bin->magic);
    }

    if (bin->magic != BIN_MAGIC)
    {
        dbgln("a:%d bin:%p m:0x%x", allocator->bin_size, bin, bin->magic);
    }
    assert(bin->magic == BIN_MAGIC);
    block->bin = bin->next;
    allocator->n_alloc++;

    return bin;
}

void m_free(void *ptr)
{
    if ((uintptr_t)ptr % PAGESIZE == 0)
    {
        block_header_t *head = allocated_block_header;
        int found = 0;
        block_header_t *prev = NULL;
        while (head)
        {
            assert(head->magic == HEADER_MAGIC);
            if (head->ptr == ptr)
            {
                found = 1;
                break;
            }
            prev = head;
            head = head->next;
        }
        if (!found)
            assert(0);

        os_free((uintptr_t)ptr, head->size);

        if (prev)
            prev->next = head->next;
        if (head == allocated_block_header)
            allocated_block_header = head->next;
        m_free(head);
        return;
    }
    malloc_block_t *block = NULL;
    uintptr_t addr = (uintptr_t)ptr & PAGEMASK;

    int b = 10;
    while (b)
    {
        block = (malloc_block_t *)addr;
        if (block->magic == BLOCK_MAGIC)
            break;
        addr -= PAGESIZE;
        b--;
    }
    assert(block->magic == BLOCK_MAGIC && "start of block not found");
    allocator_t *allocator = block->allocator;
    assert(allocator);
    assert((uintptr_t)ptr < (uintptr_t)block + allocator->block_size);
    malloc_bin_t *bin = ptr;
    bin->magic = BIN_MAGIC;
    bin->next = block->bin;
    block->bin = bin;
}

void *m_realloc(void *ptr, uint32_t size)
{
    if ((uintptr_t)ptr % PAGESIZE == 0)
    {
        block_header_t *head = allocated_block_header;
        int found = 0;
        while (head)
        {
            assert(head->magic == HEADER_MAGIC);
            if (head->ptr == ptr)
            {
                found = 1;
                break;
            }
            head = head->next;
        }
        if (!found)
        {
            assert(0);
            dbgln("ptr:%p not found", ptr);
        }

        if (head->size >= size)
            return ptr;

        if (size % PAGESIZE)
        {
            size &= PAGEMASK;
            size += PAGESIZE;
        }
        void *ret = alloc(size);
        memcpy(ret, ptr, head->size);
        os_free((uintptr_t)ptr, head->size);
        head->size = size;
        head->ptr = ret;
        return ret;
    }

    malloc_block_t *block = NULL;
    uintptr_t addr = (uintptr_t)ptr & PAGEMASK;

    int b = 10;
    while (b)
    {
        block = (malloc_block_t *)addr;
        if (block->magic == BLOCK_MAGIC)
            break;
        b--;
    }
    assert(block->magic == BLOCK_MAGIC && "start of block not found");
    allocator_t *allocator = block->allocator;
    assert(allocator);
    assert((uintptr_t)ptr < (uintptr_t)block + allocator->block_size);

    if (allocator->block_size >= size)
        return ptr;
    void *ret = alloc(size);
    memcpy(ret, ptr, allocator->block_size);

    malloc_bin_t *bin = ptr;
    bin->magic = BIN_MAGIC;
    bin->next = block->bin;
    block->bin = bin;

    return ret;
}

void __attribute__((malloc)) * malloc(size_t size)
{
    assert(size != 0);
    pthread_mutex_lock(&m_lock);
    void *ret = alloc(size);
    pthread_mutex_unlock(&m_lock);
    return ret;
}

void __attribute__((malloc)) * calloc(size_t nmemb, size_t size)
{
    assert(size * nmemb != 0);
    pthread_mutex_lock(&m_lock);
    void *ret = alloc(nmemb * size);
    pthread_mutex_unlock(&m_lock);
    memset(ret, 0, nmemb * size);
    return ret;
}

void __attribute__((malloc)) * valloc(size_t size)
{
    assert(size != 0);
    pthread_mutex_lock(&m_lock);
    if (size % PAGESIZE)
    {
        size &= PAGEMASK;
        size += PAGESIZE;
    }
    void *ret = alloc(size);
    pthread_mutex_unlock(&m_lock);
    return ret;
}

void __attribute__((malloc)) * realloc(void *ptr, size_t size)
{
    if (ptr == NULL)
        return malloc(size);
    if (size == 0)
    {
        free(ptr);
        return NULL;
    }
    pthread_mutex_lock(&m_lock);
    void *ret = m_realloc(ptr, size);
    pthread_mutex_unlock(&m_lock);
    return ret;
}

void free(void *ptr)
{
    assert(ptr != NULL);
    pthread_mutex_lock(&m_lock);
    m_free(ptr);
    pthread_mutex_unlock(&m_lock);
}
