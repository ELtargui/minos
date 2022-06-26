#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <sys/ioclt.h>
#include <gui/app.h>
#include <gui/window.h>
#include <gui/container.h>

typedef struct fb_info
{
    uint32_t paddr;
    int width;
    int hieght;
    int bpp;
} fb_info_t;

void click(widget_t *widget, int btn, int x, int y)
{
    printf("disktop click\n");
}

int main(int argc, char const *argv[])
{
    fb_info_t fb_info;
    int fb = open("/dev/fb", 0);
    if (fb == -1)
    {
        perror("open fb");
        return EXIT_FAILURE;
    }

    if (ioctl(fb, 0, &fb_info) == -1)
    {
        perror("ioctl fb");
        return EXIT_FAILURE;
    }

    app_t *app = new_application(argc, argv);
    if (!app)
    {
        printf("couldnt create app\n");
        return EXIT_FAILURE;
    }
    printf("files \n");

    window_t *win = new_window(app, fb_info.width, fb_info.hieght, 1);
    assert(win);
    close(fb);

    window_set_backgound_color(win, rgba_color(200, 200, 235, 255));
    window_set_position(win, 0, 0);

    container_t *container = new_container(NULL, 0);
    window_set_root_widget(win, &container->widget);
    container->widget.on_mouse_down = click;

    window_show(win);

    if (!fork())
    {
        const char *args[] = {
            "/bin/taskbar",
            NULL,
        };

        execve(args[0], args, NULL);
        assert(0);
    }

   
    return app_main(app);
}
