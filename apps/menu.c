#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <sys/ioclt.h>
#include <gui/app.h>
#include <gui/window.h>
#include <gui/container.h>
#include <gui/winmsg.h>

typedef struct fb_info
{
    uint32_t paddr;
    int width;
    int hieght;
    int bpp;
} fb_info_t;

void click(widget_t *widget, int btn, int x, int y)
{
    printf("menu click\n");
}

int menu_width = 300;
int menu_hieght = 500;
int dw = 0, dh = 0;
int taskbar_hieght = 0;

int main(int argc, char const *argv[])
{

    dw = atoi(argv[1]);
    dh = atoi(argv[2]);
    taskbar_hieght = atoi(argv[3]);

    printf("dw:%d dh:%d th:%d\n", dw, dh, taskbar_hieght);
    app_t *app = new_application(argc, argv);
    if (!app)
    {
        printf("couldnt create app\n");
        return EXIT_FAILURE;
    }

    window_t *win = new_window(app, menu_width, menu_hieght, 1);
    assert(win);
    window_set_position(win, 0, dh - (taskbar_hieght + menu_hieght));

    window_set_backgound_color(win, rgba_color(44, 44, 44, 255));

    container_t *container = new_container(NULL, 0);
    window_set_root_widget(win, &container->widget);
    container->widget.on_mouse_down = click;

    app_send_msg(app, SYS_MENU, NULL, 0);
    {
        char tmp[512];
        if (app_recv(app, tmp) == -1)
        {
            printf("error recv\n");
            return EXIT_FAILURE;
        }
        ipc_msg_t *msg = (ipc_msg_t *)tmp;
        if (msg->event != SYS_MENU)
        {
            printf("error ref\n");
            return EXIT_FAILURE;
        }
    }
    window_show(win);
    return app_main(app);
}
