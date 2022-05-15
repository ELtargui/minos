#include <mink/types.h>
#include <mink/mm.h>
#include <mink/lock.h>
#include <mink/string.h>
#include <mink/debug.h>

#define BLOCK_MAGIC 0xdeadc0de
#define BIN_MAGIC 0xdeadbeef
#define HEADER_MAGIC 0xfdadc0de
struct allocator;

lock_t m_lock = {0, "malloc", -1, 0};

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

allocator_t m_malloc[] = {
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

allocator_t *get_allocator(uint32_t size)
{
    for (int i = 0; i < 8; i++)
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
        void *ptr = mm_heap_alloc(size);
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
        block = (malloc_block_t *)mm_heap_alloc(allocator->block_size);
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

        mm_heap_free((uintptr_t)ptr, head->size);

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
    if ((uintptr_t)ptr == 0x1f404ff0)
        assert(0);
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
            dbgln("ptr:%p not found", ptr);
            assert(0);
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
        mm_heap_free((uintptr_t)ptr, head->size);
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
    lock(&m_lock);
    void *ret = alloc(size);
    unlock(&m_lock);
    return ret;
}

void __attribute__((malloc)) * calloc(size_t nmemb, size_t size)
{
    assert(size * nmemb != 0);
    lock(&m_lock);
    void *ret = alloc(nmemb * size);
    unlock(&m_lock);
    memset(ret, 0, nmemb * size);
    return ret;
}

void __attribute__((malloc)) * valloc(size_t size)
{
    assert(size != 0);
    lock(&m_lock);
    if (size % PAGESIZE)
    {
        size &= PAGEMASK;
        size += PAGESIZE;
    }
    void *ret = alloc(size);
    unlock(&m_lock);
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
    lock(&m_lock);
    void *ret = m_realloc(ptr, size);
    unlock(&m_lock);
    return ret;
}

void free(void *ptr)
{
    assert(ptr != NULL);
    lock(&m_lock);
    m_free(ptr);
    unlock(&m_lock);
}
