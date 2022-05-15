#include <mink/types.h>
#include <mink/i386.h>
#include <mink/process.h>
#include <mink/debug.h>
#include <mink/mm.h>
#include <mink/region.h>
#include <mink/string.h>
#include <mink/fs.h>
#include <mink/multiboot.h>

static fsnode_t *fb;

vbe_info_t vbe_info;

typedef struct fb_info
{
    uint32_t paddr;
    int width;
    int hieght;
    int bpp;
} fb_info_t;

int fb_map(struct fsnode *node, region_t *region, off_t off, size_t size)
{
    assert(node == fb);

    (void)off;
    (void)size;

    int frame = vbe_info.physbase;
    for (uintptr_t i = region->base; i < region->base + region->size; i += PAGESIZE, frame += PAGESIZE)
    {
        vmpage_t *page = make_vmpage(current_process()->vmdir, i);
        page->frame = frame / PAGESIZE;
        frame_set_allocated(frame, True);
        page->present = 1;
        page->user = 1;
        page->cachedisable = 1;
        page->writethrough = 1;
        page->rw = 1;
    }

    //direct map
    frame = vbe_info.physbase;
    for (uintptr_t i = vbe_info.physbase; i < vbe_info.physbase + vbe_info.Xres * vbe_info.Yres * 4; i += PAGESIZE, frame += PAGESIZE)
    {
        vmpage_t *page = make_vmpage(current_process()->vmdir, i);
        page->frame = frame / PAGESIZE;
        frame_set_allocated(frame, True);
        page->present = 1;
        page->user = 1;
        page->rw = 1;
        page->cachedisable = 1;
        page->writethrough = 1;
    }

    // inavalidate_region(region->base, region->size);
    inavalidate_all();
    return 0;
}

int fb_ioctl(fsnode_t *node, int cmd, void *a, void *b)
{
    (void)node;
    (void)b;

    switch (cmd)
    {
    case 0:
    {
        fb_info_t *info = a;
        info->paddr = vbe_info.physbase;
        info->width = vbe_info.Xres;
        info->hieght = vbe_info.Yres;
        info->bpp = vbe_info.bpp;
    }
    break;
    default:
        return -1;
    }
    return 0;
}

static fsnode_ops_t fb_ops = {
    .map = fb_map,
    .ioctl = fb_ioctl,
};

void fb_install(vbe_info_t *info)
{
    vbe_info = *info;

    fb = calloc(1, sizeof(fsnode_t));
    strcpy(fb->name, "fb");
    fb->inode = vbe_info.physbase;
    fb->type = FS_BLK;
    fb->ops = &fb_ops;

    vfs_bind("/dev/fb", fb, 0666);
    dbgln("fb[%p %dx%dx%d]", vbe_info.physbase, vbe_info.Xres, vbe_info.Yres, vbe_info.bpp);
}
