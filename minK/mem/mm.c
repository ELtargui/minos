#include <mink/types.h>
#include <mink/multiboot.h>
#include <mink/stdio.h>
#include <mink/i386.h>
#include <mink/debug.h>
#include <mink/string.h>
#include <mink/mm.h>
#include <mink/process.h>
#include <mink/region.h>
#include <mink/lock.h>

#define MEMORY_AVAILABLE 1
#define MEMORY_RESERVED 2
#define MEMORY_ACPI_RECLAIMABLE 3
#define MEMORY_NVS 4
#define MEMORY_BADRAM 5

#define HEAP_START 0x1F400000
#define HEAP_INIT_SIZE 0x400000
#define HEAP_MAX_SIZE 0x6000000

extern void *kernel_end;
extern void ramdisk(uintptr_t start, size_t size);

static size_t mem_size = 0;
static uintptr_t mm_alloc_ptr = 0;
static uint32_t *mm_bitmap = NULL;
static uint32_t mm_framse_cnt = 0;

static uintptr_t mm_heap_ptr = 0;
static uintptr_t mm_heap_avialable = 0;

static lock_t frames_lock;

void frame_set_allocated_unlocked(uint32_t frame, int b)
{
    frame /= PAGESIZE;

    if (frame > mm_framse_cnt)
    {
        return;
    }

    int i = frame / 32;
    int j = frame % 32;

    if (b)
    {
        if (mm_bitmap[i] & (1u << j))
        {
            dbgln("FRAME:%p alredy allocated", frame * PAGESIZE);
            assert(0 && "set frame");
        }
        mm_bitmap[i] |= (1u << j);
    }
    else
    {
        if (!(mm_bitmap[i] & (1u << j)))
        {
            dbgln("FRAME:%p alredy free", frame * PAGESIZE);
            assert(0 && "clear frame");
        }
        mm_bitmap[i] &= ~(1u << j);
    }
}

void frame_set_allocated(uint32_t frame, int b)
{
    lock(&frames_lock);
    frame_set_allocated_unlocked(frame, b);
    unlock(&frames_lock);
}

uint32_t alloc_frame(int cnt)
{
    lock(&frames_lock);
    assert(cnt == 1);
    for (uint32_t i = 0; i < mm_framse_cnt / 32; i++)
    {
        if (mm_bitmap[i] != 0xffffffff)
        {
            for (int j = 0; j < 32; j++)
            {

                if (!(mm_bitmap[i] & (1u << j)))
                {
                    uint32_t frame = (i * 32 + j) * PAGESIZE;
                    frame_set_allocated_unlocked(frame, True);
                    unlock(&frames_lock);

                    return frame;
                }
            }
        }
    }
    ERROR("out of memory");
    return 0;
}

void *mm_alloc(size_t size)
{
    if (size % PAGESIZE)
    {
        size &= PAGEMASK;
        size += PAGESIZE;
    }

    void *ret;
    if (mm_heap_ptr)
    {
        ret = valloc(size);
    }
    else
    {
        ret = (void *)mm_alloc_ptr;
        mm_alloc_ptr += size;
    }

    memset(ret, 0, size);
    return ret;
}

uintptr_t mm_get_paddr(uintptr_t addr)
{
    if (!mm_heap_ptr)
        return addr;
    thread_t *thread = current_thread();
    return vm_paddr(thread ? thread->cpu.vmdir : kernel_vmdir, addr);
}

void mm_page_fault(regs_t *r)
{
    interrupt_disable();
    // static int in_fault = 0;
    // if(in_fault)
    //     assert(0  && "in_fault");
    // in_fault = 1;

    uint32_t address, vmdirp;
    asm volatile("mov %%cr2, %0"
                 : "=r"(address));
    asm volatile("mov %%cr3, %0"
                 : "=r"(vmdirp));

    int present = !(r->error & 0x1) ? 1 : 0;
    int rw = r->error & 0x2 ? 1 : 0;
    int user = r->error & 0x4 ? 1 : 0;
    int in_kernel = !(r->cs & 3);

    if (!in_kernel)
    {
        region_t *region = get_region(current_process(), address);
        if (region && !region->free)
        {
            if (region->cow)
            {
                if (region_handle_cow_fault(region, address))
                {
                    assert(0 && "sig fault");
                }
                // in_fault = 0;
                interrupt_resume();
                return;
            }
        }
    }

    int tid = current_thread() ? current_thread()->id : 0;
    int pid = current_process() ? current_process()->pid : 0;
    char *name = current_process() ? current_process()->name : NULL;
    
    printf("[(%s)%d/%d]page fault @%p eip:%p [p:%d rw:%d u:%d kernel:%d cr3:%p]\n", name, pid, tid,
           address, r->eip, present, rw, user, in_kernel, vmdirp);

    if (in_kernel)
    {
        symbole_t *sym = symbole_from_address(r->eip);
        if (sym)
            printf("    in %s()\n", sym->name);
    }
    assert(0 && "page fault");
}

void install_mem(void *bootinfo)
{
    multiboot_t *mboot = bootinfo;
    mm_alloc_ptr = (uintptr_t)&kernel_end;
    mboot_mod_t *modules = (mboot_mod_t *)mboot->mods_addr;

    for (uint32_t i = 0; i < mboot->mods_count; i++)
    {
        printf("module [%p:%p] cmd:%s\n", modules->mod_start, modules->mod_end, modules->cmdline);

        if (!strcmp((const char *)modules->cmdline, "--ramdisk"))
            ramdisk(modules->mod_start, modules->mod_end - modules->mod_start);

        if (modules->mod_end > mm_alloc_ptr)
            mm_alloc_ptr = modules->mod_end;
        modules++;
    }

    if (mm_alloc_ptr % PAGESIZE)
    {
        mm_alloc_ptr &= PAGEMASK;
        mm_alloc_ptr += PAGESIZE;
    }

    size_t modules_size = mm_alloc_ptr - (uintptr_t)&kernel_end;
    if (modules_size % PAGESIZE)
    {
        modules_size &= PAGEMASK;
        modules_size += PAGESIZE;
    }

    printf("mem start:%p\n", mm_alloc_ptr);

    mem_size = mboot->mem_lower + mboot->mem_upper;
    printf("memsize : %d kb modules size : %d\n", mem_size, modules_size);

    mm_framse_cnt = mem_size / 4;
    mm_bitmap = mm_alloc((mm_framse_cnt / (sizeof(uint32_t) * 8)) * sizeof(uint32_t));

    if (mboot->flags & 0x40)
    {
        for (uint32_t i = 0; i < mboot->mmap_length; i += sizeof(mboot_mmap_t))
        {
            mboot_mmap_t *mmap = (mboot_mmap_t *)(mboot->mmap_addr + i);
            {
                printf("mem [T:%d 0x%x%x:%x%x] ", mmap->type, mmap->base_addr, mmap->length);
            }
            if (mmap->type == MEMORY_RESERVED)
            {
                printf("reserved");
                for (uint64_t m = mmap->base_addr; m < mmap->length; m += PAGESIZE)
                {
                    if (m > 0xffffffff)
                        break;
                    frame_set_allocated(m, True);
                }
            }
            printf("\n");
            mmap = (mboot_mmap_t *)((uintptr_t)mmap + mmap->size + sizeof(uint32_t));
        }
    }

    kernel_vmdir = mm_alloc(sizeof(vmdir_t));

    make_vmpage(kernel_vmdir, 0)->present = False;
    frame_set_allocated(0, True);

    uintptr_t alloc_end = mm_alloc_ptr + PAGESIZE * (2 + HEAP_MAX_SIZE / (4 * 0x100000));
    uintptr_t i;

    for (i = PAGESIZE; i < alloc_end; i += PAGESIZE)
    {
        vm_map_page(make_vmpage(kernel_vmdir, i), 0, 0, i);
    }

    for (i = HEAP_START; i < HEAP_START + HEAP_INIT_SIZE; i += PAGESIZE)
    {
        vm_alloc_page(make_vmpage(kernel_vmdir, i), 0, 1);
    }

    for (i = HEAP_START + HEAP_INIT_SIZE; i < HEAP_START + HEAP_MAX_SIZE; i += PAGESIZE)
    {
        make_vmpage(kernel_vmdir, i);
    }

    install_isr_handler(14, mm_page_fault);
    vm_flush_vmdir(kernel_vmdir);

    dbgln("paging enabled");
    mm_heap_ptr = HEAP_START;
    mm_heap_avialable = HEAP_INIT_SIZE;

    make_vmpage(kernel_vmdir, 0xfeedf000);
    init_lock(&frames_lock, "frames");
    // init_lock(&heap_lock, "frames");
}

typedef struct mm_heap_block
{
    uint32_t magic;
    uint32_t size;
    struct mm_heap_block *prev;
    struct mm_heap_block *next;
} mm_heap_block_t;

#define HEAP_BMAGIC 0xC001C0DE
mm_heap_block_t *mm_free_heap_block_list_head = NULL;

void *mm_heap_alloc(size_t size)
{
    assert(size);
    if (size % PAGESIZE)
    {
        size &= PAGEMASK;
        size += PAGESIZE;
    }

    if (mm_free_heap_block_list_head)
    {
        mm_heap_block_t *block = mm_free_heap_block_list_head;
        while (block)
        {
            assert(block->magic == HEAP_BMAGIC);
            if (block->size >= size)
            {
                void *ret = NULL;
                if (block->size > size)
                {
                    block->size -= size;
                    ret = (void *)((uintptr_t)block + block->size);
                }
                else
                {
                    ret = block;
                    if (block->prev)
                        block->prev->next = block->next;
                    if (block->next)
                        block->next->prev = block->prev;
                    if (block == mm_free_heap_block_list_head)
                        mm_free_heap_block_list_head = block->next;
                }

                for (uintptr_t i = (uintptr_t)ret; i < (uintptr_t)ret + size; i += PAGESIZE)
                {
                    vmpage_t *page = get_vmpage(kernel_vmdir, i);
                    assert(page);
                    page->rw = 1;
                }
                inavalidate_region((uintptr_t)ret, size);
                return ret;
            }
            block = block->next;
        }
    }
    void *ret = (void *)mm_heap_ptr;

    if (mm_heap_avialable < size)
    {
        uintptr_t heap = mm_heap_ptr;
        while (mm_heap_avialable < size)
        {
            if (mm_heap_ptr + mm_heap_avialable > HEAP_START + HEAP_MAX_SIZE)
            {
                assert(0 && HEAP_MAX_SIZE);
            }

            for (uintptr_t i = heap; i < heap + MB; i += PAGESIZE)
            {
                vm_alloc_page(get_vmpage(kernel_vmdir, i), 0, 1);
            }
            heap += MB;
            mm_heap_avialable += MB;
        }
    }

    mm_heap_ptr += size;
    mm_heap_avialable -= size;
    memset(ret, 0xaa, size);
    return ret;
}

void mm_heap_free(uintptr_t ptr, size_t size)
{
    if (ptr <= 0x1f405000 && ptr + size >= 0x1f405000)
        assert(0);
    assert(size % PAGESIZE == 0);
    mm_heap_block_t *block = (mm_heap_block_t *)ptr;
    block->magic = HEAP_BMAGIC;
    block->size = size;
    block->prev = NULL;
    block->next = mm_free_heap_block_list_head;

    for (uintptr_t i = ptr; i < ptr + size; i += PAGESIZE)
    {
        vmpage_t *page = get_vmpage(kernel_vmdir, i);
        assert(page);
        page->rw = 0;
    }

    inavalidate_region(ptr, size);

    if (mm_free_heap_block_list_head)
        mm_free_heap_block_list_head->prev = block;
    mm_free_heap_block_list_head = block;
}
