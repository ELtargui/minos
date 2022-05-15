#pragma once
#include <mink/types.h>
#include <mink/process.h>

typedef struct region
{
    uintptr_t base;
    size_t size;

    uint8_t type;
    uint8_t __aling;

    uint16_t free : 1;
    uint16_t user : 1;
    uint16_t readable : 1;
    uint16_t writable : 1;
    uint16_t executable : 1;
    uint16_t cow : 1;
    uint16_t __u : 10;

    void *ctx;
    char *name;
} region_t;

typedef struct region_cow
{
    uint8_t *frames_ref;
    int ref;
    lock_t lock;
} region_cow_t;

region_t *get_region(process_t *process, uintptr_t address);
region_t *alloc_region(process_t *process, uintptr_t base, size_t size, int fixed);
int region_map(process_t *process, region_t *region, int u, int r, int w, int x);
void region_set_name(region_t *region, const char *name);
int region_child_cow(process_t *parent, process_t *child, region_t *region);
int region_handle_cow_fault(region_t *region, uintptr_t address);

void region_free_impl(process_t *process, region_t *region);
void merge_free_regions(list_t *regions);
