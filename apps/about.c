#include <stdio.h>
#include <stdlib.h>
#include <gui/app.h>
#include <gui/window.h>
#include <gui/container.h>
#include <gui/label.h>
#include <gui/button.h>

void btn_click(button_t *btn)
{
    printf("==(%s)==\n", (char *)btn->user);
}

int main(int argc, char const *argv[])
{
    app_t *app = new_application(argc, argv);
    if (!app)
    {
        printf("couldnt create app\n");
        return EXIT_FAILURE;
    }

    window_t *win = new_window(app, 430, 100, 0);
    if (!win)
    {
        printf("error create window\n");
        return EXIT_FAILURE;
    }
    window_set_backgound_color(win, rgba_color(150, 150, 250, 255));
    window_set_position(win, 50, 100);

    widget_set_layout(win->root, 2, 0);

    label_t *minos = new_label(win->root, "minOS is a unix like os 0123456789", "minOS");
    // label_t *minos = new_label(win->root, ",", "minOS");
    label_set_fontsize(minos, 16);

    // new_label(win->root, "a small unix like operating system", "text");
    // button_t *btn = new_button(win->root, "exit", "exit");

 
    // button_on_click(btn, btn_click, 0);

    window_show(win);
    return app_main(app);
}
 