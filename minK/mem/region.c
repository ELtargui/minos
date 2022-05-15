#include <mink/types.h>
#include <mink/stdio.h>
#include <mink/i386.h>
#include <mink/debug.h>
#include <mink/mm.h>
#include <mink/region.h>
#include <mink/process.h>
#include <mink/string.h>
#include <errno.h>

region_t *get_region(process_t *process, uintptr_t address)
{
    assert(process);
    foreach (process->regions, n)
    {
        region_t *r = n->value;
        if (address >= r->base && address < r->base + r->size)
            return r;
    }

    return NULL;
}

region_t *alloc_region(process_t *process, uintptr_t base, size_t size, int fixed)
{
    if (size % PAGESIZE)
    {
        size &= PAGEMASK;
        size += PAGESIZE;
    }

    region_t *region = NULL;
    if (fixed)
    {
        assert(base % PAGESIZE == 0);
        region_t *r = get_region(process, base);
        assert(r);
        assert(r->free == True);

        if (r->base == base)
        {
            region = r;
        }
        else //if (region->base > r->base)
        {
            assert(base > r->base);

            region = calloc(1, sizeof(region_t));
            region->base = base;
            region->size = r->size - (base - r->base);
            region->free = 1;

            r->size = base - r->base;

            list_add_after(process->regions, r, region);
        }
    }
    else
    {
        foreach (process->regions, n)
        {
            region_t *r = n->value;
            if (r->free && r->size >= size)
            {
                region = r;
                break;
            }
        }
    }

    assert(region);

    if (region->size > size)
    {
        region_t *r = calloc(1, sizeof(region_t));
        r->base = region->base + size;
        r->size = region->size - size;
        r->free = True;
        list_add_after(process->regions, region, r);
        region->size = size;
    }

    if (fixed)
        assert(region->base == base);
    assert(region->size == size);
    region->free = False;
    return region;
}

void region_free_cow(process_t *process, region_t *region)
{
    assert(region->cow);
    region_cow_t *rcow = region->ctx;
    assert(rcow);
    assert(rcow->ref > 0);
    int index = 0;
    for (uintptr_t i = region->base; i < region->base + region->size; i += PAGESIZE)
    {
        vmpage_t *page = get_vmpage(process->vmdir, i);
        if (page && page->present)
        {
            if (page->rw)
            {
                vm_free_page(page);
            }
            else
            {
                page->present = 0;
                page->user = 0;
                page->frame = 0;
                rcow->frames_ref[index]--;
            }
        }
        index++;
    }

    // rcow->ref--;

    if (--rcow->ref == 0)
    {
        dbgln("free cow");
        free(rcow->frames_ref);
        free(rcow);
    }

    region->cow = 0;
    region->ctx = NULL;
}

void region_free_impl(process_t *process, region_t *region)
{
    assert(region->free == 0);
    if (region->cow)
        region_free_cow(process, region);
    else
    {
        for (uintptr_t i = region->base; i < region->base + region->size; i += PAGESIZE)
        {
            vmpage_t *page = get_vmpage(process->vmdir, i);
            if (page && page->present)
            {
                vm_free_page(page);
            }
        }
    }

    region->free = 1;
    region_set_name(region, NULL);
    inavalidate_region(region->base, region->size);
}

void merge_free_regions(list_t *regions)
{
    assert(regions);

    list_node_t *node = regions->head;
    while (node)
    {
        list_node_t *next = node->next;
        if (!next)
            break;
        region_t *a = node->value;
        region_t *b = next->value;
        if (a->free && b->free)
        {
            a->size += b->size;
            list_remove_node(regions, next);
            region_set_name(b, NULL);
            free(b);
            continue;
        }
        node = node->next;
    }
}

int region_map(process_t *process, region_t *region, int u, int r, int w, int x)
{
    region->user = u ? 1 : 0;
    region->readable = r ? 1 : 0;
    region->writable = w ? 1 : 0;
    region->executable = x ? 1 : 0;

    for (uintptr_t i = region->base; i < region->base + region->size; i += PAGESIZE)
    {
        vm_alloc_page(make_vmpage(process->vmdir, i), region->user, region->writable);
    }
    inavalidate_region(region->base, region->size);
    // inavalidate_all();
    return 0;
}

void region_set_name(region_t *region, const char *name)
{
    if (region->name)
        free(region->name);
    if (name)
        region->name = strdup(name);
    else
        region->name = NULL;
}

int region_child_cow(process_t *parent, process_t *child, region_t *region)
{
    assert(region->free == False);
    region_t *cregion = alloc_region(child, region->base, region->size, True);
    assert(cregion);

    if (region->cow)
    {
        region_cow_t *rcow = region->ctx;
        dbgln("parent region already in cow : %d", rcow->ref);
        lock(&rcow->lock);
        int index = 0;
        for (uintptr_t i = region->base; i < region->base + region->size; i += PAGESIZE)
        {
            vmpage_t *parent_page = get_vmpage(parent->vmdir, i);
            if (parent_page && parent_page->present)
            {
                parent_page->rw = 0;

                vmpage_t *child_page = make_vmpage(child->vmdir, i);
                assert(child_page);
                *child_page = *parent_page;
                rcow->frames_ref[index]++;
            }
            else
            {
                assert(0);
            }
            index++;
        }
        rcow->ref++;

        cregion->user = 1;
        cregion->readable = region->readable;
        cregion->writable = region->writable;
        cregion->executable = region->executable;
        cregion->cow = 1;
        cregion->ctx = rcow;
        region_set_name(cregion, region->name);
        unlock(&rcow->lock);
        return 0;
    }

    region_cow_t *rcow = malloc(sizeof(region_cow_t));
    rcow->ref = 2;
    rcow->frames_ref = calloc(region->size / PAGESIZE, sizeof(uint8_t));
    init_lock(&rcow->lock, "cow");

    lock(&rcow->lock);
    int index = 0;
    for (uintptr_t i = region->base; i < region->base + region->size; i += PAGESIZE)
    {
        vmpage_t *parent_page = get_vmpage(parent->vmdir, i);
        if (parent_page && parent_page->present)
        {
            parent_page->rw = 0;

            vmpage_t *child_page = make_vmpage(child->vmdir, i);
            assert(child_page);
            *child_page = *parent_page;
            rcow->frames_ref[index]++;
        }
        else
        {
            assert(0);
        }
        index++;
    }

    region->cow = 1;
    region->ctx = rcow;
    cregion->user = 1;
    cregion->readable = region->readable;
    cregion->writable = region->writable;
    cregion->executable = region->executable;

    cregion->cow = 1;
    cregion->ctx = rcow;
    region_set_name(cregion, region->name);
    inavalidate_region(region->base, region->size);
    unlock(&rcow->lock);

    return 0;
}

int region_handle_cow_fault(region_t *region, uintptr_t address)
{
    assert(interrupt_state());
    assert(region);
    assert(region->free == 0);
    assert(region->cow);
    assert(region->writable);
    assert(address >= region->base && region->base + region->size > address);

    vmpage_t *page = get_vmpage(current_process()->vmdir, address);
    assert(page);
    assert(page->present);
    assert(page->rw == 0);

    page->rw = 1;

    address &= PAGEMASK;
    int index = (address - region->base) / PAGESIZE;
    region_cow_t *rcow = region->ctx;
    assert(rcow);

    if (rcow->ref == 1)
    {
        for (uintptr_t i = region->base; i < region->base + region->size; i += PAGESIZE)
        {
            vmpage_t *page = get_vmpage(current_process()->vmdir, i);
            page->rw = 1;
        }
        region->cow = 0;
        free(rcow->frames_ref);
        free(rcow);
        region->ctx = NULL;
        inavalidate_region(region->base, region->size);

        return 0;
    }

    if (rcow->frames_ref[index] == 0)
    {
        // dbgln("@%p frame:%p org", address, page->frame * PAGESIZE);
        inavalidate_page(address);

        return 0;
    }

    assert(rcow->frames_ref[index] >= 1);

    vmpage_t *tmp = make_vmpage(current_process()->vmdir, 0xfeedf000);
    assert(tmp);
    vm_alloc_page(tmp, 1, 1);

    inavalidate_page(0xfeedf000);
    memcpy((void *)0xfeedf000, (void *)address, PAGESIZE);

    page->frame = tmp->frame;
    *tmp = (vmpage_t){0};

    // dbgln("@%p new frame:%p", address, page->frame * PAGESIZE);

    inavalidate_page(0xfeedf000);
    inavalidate_page(address);

    rcow->frames_ref[index]--;

    return 0;
}
