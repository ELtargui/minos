#include <stdio.h>
#include <stdlib.h>
#include <gui/app.h>
#include <gui/window.h>


int main(int argc, char const *argv[])
{
    app_t *app = new_application(argc, argv);
    if (!app)
    {
        printf("couldnt create app\n");
        return EXIT_FAILURE;
    }

    window_t *win = new_window(app, 100, 200, 0);
    window_set_position(win, 10, 10);
    window_set_backgound_color(win, rgba_color(255, 0, 0, 255));
    window_show(win);

    return app_main(app);
}
