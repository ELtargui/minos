#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>
#include <pthread.h>
#include <time.h>
#include <sys/ioclt.h>
#include <sys/mman.h>
#include <min/gfx.h>
#include <min/truetype.h>
#include "window.h"

typedef struct fb_info
{
    uint32_t paddr;
    int width;
    int hieght;
    int bpp;
} fb_info_t;

static gfx_t *gfx;
fb_info_t fb_info;
surface_t *mouse_shape;

int mouse_x;
int mouse_y;

list_t *wins_stack = NULL;
pthread_mutex_t stack_mutex;

void stack_add_window(s_window_t *win)
{
    pthread_mutex_lock(&stack_mutex);
    list_append(wins_stack, win);
    pthread_mutex_unlock(&stack_mutex);
}

void stack_remove_window(s_window_t *win)
{
    pthread_mutex_lock(&stack_mutex);
    assert(list_remove(wins_stack, win) == 0);
    pthread_mutex_unlock(&stack_mutex);
}

void set_mouse_pos(int x, int y)
{
    mouse_x = x;
    mouse_y = y;
}

void get_mouse_pos(int *x, int *y)
{
    *x = mouse_x;
    *y = mouse_y;
}

s_window_t *top_window_in_pos(int x, int y)
{
    pthread_mutex_lock(&stack_mutex);
    foreach_r(wins_stack, n)
    {
        s_window_t *w = n->value;
        if (!point_in_rect(x, y, win_rect(w)))
        {
            pthread_mutex_unlock(&stack_mutex);
            return w;
        }
    }
    pthread_mutex_unlock(&stack_mutex);
    return NULL;
}

void screen_redraw_rect(rect_t r)
{
    pthread_mutex_lock(&stack_mutex);

    foreach (wins_stack, n)
    {
        s_window_t *win = n->value;
        rect_t wr = win_rect(win);
        rect_t x = rect_get_intersection(wr, r);

        if (x.w != -1 && x.h != -1)
        {
            x.x -= win->x;
            x.y -= win->y;
            win_redraw_rect(win, &x);
        }
    }
    pthread_mutex_unlock(&stack_mutex);
}

void *compositor(void *arg)
{
    (void)arg;

    static int mx = 0, my = 0;
    while (1)
    {
        int redraw_mouse = 0;
        int m_x = mouse_x;
        int m_y = mouse_y;

        if (mx != m_x || my != m_y)
        {
            screen_redraw_rect(Rectangle(mx, my, mouse_shape->w, mouse_shape->h));
            mx = m_x;
            my = m_y;
            redraw_mouse++;
        }

        foreach (wins_stack, n)
        {
            s_window_t *win = n->value;
            while (win->rects->head)
            {
                rect_t *r = list_take_first(win->rects);

                int x = win->x + r->x;
                int y = win->y + r->y;
                gfx_draw_surface_rect(gfx, x, y, win->surface, r);
                r->x = x;
                r->y = y;
                gfx_add_clip(gfx, r);
            }
        }

        rect_t mouse_r = Rectangle(mx, my, mouse_shape->w, mouse_shape->h);
        foreach (gfx->clips, n)
        {
            rect_t *r = n->value;
            rect_t x = rect_get_intersection(mouse_r, *r);

            if (x.w != -1 && x.h != -1)
            {
                redraw_mouse++;
            }
        }
        gfx_blit_clips(gfx);

        if (redraw_mouse)
        {
            gfx_draw_surface_alpha(gfx, mx, my, mouse_shape);
            gfx_blit_rect(gfx, &mouse_r);
            redraw_mouse = 0;
        }

        struct timespec req;
        req.tv_sec = 0;
        // req.tv_nsec = 16500000;
        req.tv_nsec = 15600000;
        nanosleep(&req, NULL);
    }
    return NULL;
}

gfx_t *start_compositor()
{
    int fb = open("/dev/fb", 0);
    if (fb == -1)
    {
        perror("open fb");
        return NULL;
    }

    if (ioctl(fb, 0, &fb_info) == -1)
    {
        perror("ioctl fb");
        return NULL;
    }

    void *buffer = mmap(NULL, fb_info.width * fb_info.hieght * 4, PROT_WRITE, MAP_PRIVATE, fb, 0);
    if (buffer == MAP_FAILED)
    {
        perror("mmap");
        return NULL;
    }

    buffer = (void *)fb_info.paddr;
    printf("frame buffer:%p\n", buffer);
    gfx = gfx_ctx(fb_info.width, fb_info.hieght, buffer);
    gfx_fill(gfx, rgba_color(255, 0, 255, 255));

    buffer = mmap(NULL, fb_info.width * fb_info.hieght * 4, PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0);
    if (buffer == MAP_FAILED)
    {
        perror("mmap");
        return NULL;
    }
    printf("back buffer:%p\n", buffer);
    gfx_set_back_buffer(gfx, buffer);
    gfx_fill(gfx, rgba_color(255, 255, 255, 255));
    gfx_blit(gfx);

    wins_stack = new_list();
    pthread_mutex_init(&stack_mutex, NULL);

    mouse_shape = load_png("/usr/share/cursor/normal.png");
    set_mouse_pos(gfx->w / 2, gfx->h / 2);

    pthread_t t;
    if (pthread_create(&t, NULL, compositor, gfx) == -1)
    {
        perror("start thread");
    }
    return gfx;
}
