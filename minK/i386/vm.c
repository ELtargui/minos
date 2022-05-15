#include <mink/types.h>
#include <mink/i386.h>
#include <mink/debug.h>
#include <mink/string.h>
#include <mink/mm.h>

vmdir_t *kernel_vmdir = NULL;

static vmpage_t *get_vmpage_impl(vmdir_t *dir, uintptr_t vaddr, int make)
{
    vaddr /= PAGESIZE;
    uint32_t t = vaddr / 1024;
    uint32_t p = vaddr % 1024;

    if (dir->tables[t] == NULL)
    {
        if (make)
        {
            dir->tables[t] = mm_alloc(sizeof(vmtable_t));
            dir->ptrs[t] = 7 | mm_get_paddr((uintptr_t)dir->tables[t]);
        }
        else
        {
            return NULL;
        }
    }

    return &dir->tables[t]->pages[p];
}

vmpage_t *get_vmpage(vmdir_t *dir, uintptr_t vaddr)
{
    return get_vmpage_impl(dir, vaddr, False);
}

vmpage_t *make_vmpage(vmdir_t *dir, uintptr_t vaddr)
{
    return get_vmpage_impl(dir, vaddr, True);
}

uintptr_t vm_paddr(vmdir_t *dir, uintptr_t vaddr)
{
    assert(dir);
    uint32_t address = (uint32_t)vaddr;
    int t = (address / PAGESIZE) / 1024;
    int p = (address / PAGESIZE) % 1024;

    if (dir->tables[t] == NULL)
    {
        dbgln("address %p : table not present", vaddr);
        assert(0);
    }
    vmpage_t *page = &dir->tables[t]->pages[p];

    if (!page || page->frame == 0)
    {
        dbgln("address %p : table / frame not present", address);
        assert(0);
    }

    if (!page->present)
    {
        dbgln("address %p : page not present", vaddr);
        assert(0);
    }

    return (page->frame * PAGESIZE) + (address % PAGESIZE);
}

void vm_map_page(vmpage_t *page, int u, int rw, uint32_t paddr)
{
    assert(page);
    frame_set_allocated(paddr, True);
    page->present = 1;
    page->user = u;
    page->rw = rw;
    page->frame = paddr / PAGESIZE;
}

void vm_free_page(vmpage_t *page)
{
    frame_set_allocated(page->frame * PAGESIZE, 0);
    page->present = 0;
    page->user = 0;
    page->rw = 0;
    page->frame = 0;
    page->cachedisable = 0;
    page->writethrough = 0;
}

uint32_t vm_alloc_page(vmpage_t *page, int u, int rw)
{
    assert(page);
    uint32_t frame = alloc_frame(1);
    page->present = 1;
    page->user = u;
    page->rw = rw;
    page->frame = frame / PAGESIZE;
    return frame;
}

vmdir_t *vm_new_vmdir()
{
    assert(kernel_vmdir);
    vmdir_t *dir = mm_alloc(sizeof(vmdir_t));
    for (int i = 0; i < 1024; i++)
    {
        if (kernel_vmdir->tables[i])
        {
            dir->tables[i] = kernel_vmdir->tables[i];
            dir->ptrs[i] = kernel_vmdir->ptrs[i];
        }
    }
    return dir;
}

void inavalidate_page(uint32_t vaddr)
{
    asm volatile(
        "movl %0, %%eax\n"
        "invlpg (%%eax)\n" ::"r"(vaddr)
        : "%eax");
}

void inavalidate_region(uint32_t vaddr, uint32_t size)
{
    for (uint32_t i = vaddr; i < vaddr + size; i += PAGESIZE)
    {
        asm volatile(
            "movl %0, %%eax\n"
            "invlpg (%%eax)\n" ::"r"(i)
            : "%eax");
    }
}

void inavalidate_all()
{
    asm volatile(
        "movl %%cr3, %%eax\n"
        "movl %%eax, %%cr3\n" ::
            : "%eax");
}

void vm_flush_vmdir(vmdir_t *dir)
{
    assert(dir);
    uint32_t ptr = mm_get_paddr((uintptr_t)dir->ptrs);
    asm volatile(
        "mov %0, %%cr3\n"
        "mov %%cr0, %%eax\n"
        "orl $0x80000000, %%eax\n"
        "mov %%eax, %%cr0\n" ::"r"(ptr)
        : "%eax");
}
