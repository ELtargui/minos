#include <stdio.h>
#include <stdlib.h>
#include <gui/app.h>
#include <gui/window.h>
#include <gui/container.h>
#include <gui/label.h>
#include <gui/button.h>
#include <ctype.h>

int state = 0;
char buffer[128];
char result[128];
int idx = 0;
label_t *display;

typedef struct val
{
    int type;
    union num
    {
        float f;
        int i;
    } as;
} val_t;

typedef struct parser
{
    char *buf;
    int size;
    int id;
} parser_t;

void error(char *e)
{
    label_set_text(display, e, 14);
    widget_update(&display->widget);
}

val_t parse_add(parser_t *parser);

int current(parser_t *parser)
{
    return parser->buf[parser->id];
}

int advence(parser_t *parser)
{
    int c = parser->buf[parser->id];
    parser->id++;
    return c;
}

val_t parse_num(parser_t *parser)
{
    int r = 0;
    int f = 0;
    int fd = 10;
    val_t v;

    while (current(parser) && isdigit(current(parser)))
    {
        r = r * 10 + (current(parser) - '0');
        advence(parser);
    }
    printf("c::%c\n", current(parser));
    if (current(parser) == '.')
    {
        advence(parser);
        while (current(parser) && isdigit(current(parser)))
        {
            f = f * 10 + (current(parser) - '0');
            fd *= 10;
            advence(parser);
        }
    }

    if (f)
    {
        v.type = 1;
        v.as.f = r + (float)f / fd;
    }
    else
    {
        v.type = 0;
        v.as.i = r;
    }

    return v;
}

val_t parse_paren(parser_t *parser)
{
    if (current(parser) == '(')
    {
        val_t v = parse_add(parser);
        if (current(parser) != ')')
        {
            error("expected ')'");
            v.type = -1;
            return v;
        }
        advence(parser);
        return v;
    }

    return parse_num(parser);
}

val_t parse_mul(parser_t *parser)
{
    val_t a = parse_paren(parser);
    if (a.type == -1)
        return a;
    if (current(parser) == 'x' || current(parser) == '/')
    {
        int op = advence(parser);
        val_t b = parse_paren(parser);
        if (b.type == -1)
            return b;

        if (op == 'x')
        {
            if (a.type == 1)
                a.as.f = a.as.f * b.as.f;
            else
                a.as.i = a.as.i * b.as.i;
        }
        else
        {
            if (a.type == 1)
                a.as.f = a.as.f / b.as.f;
            else
            {
                if (a.as.i % b.as.i)
                {
                    a.type = 1;
                    a.as.f = (float)a.as.i / (float)b.as.i;
                }
                else
                {
                    a.as.i = a.as.i / b.as.i;
                }
            }
        }
    }

    return a;
}

val_t parse_add(parser_t *parser)
{
    val_t a = parse_mul(parser);
    if (a.type == -1)
        return a;

    if (current(parser) == '+' || current(parser) == '-')
    {
        int op = advence(parser);
        val_t b = parse_mul(parser);
        if (b.type == -1)
            return b;

        printf("%d %c %d:", a.as.i, op, b.as.i);
        if (op == '+')
        {
            if (a.type == 1)
                a.as.f = a.as.f + b.as.f;
            else
                a.as.i = a.as.i + b.as.i;
        }
        else
        {
            if (a.type == 1)
                a.as.f = a.as.f - b.as.f;
            else
                a.as.i = a.as.i - b.as.i;
        }
    }
    printf("(%d) \n", a.as.i);
    return a;
}

void button_click(button_t *btn)
{
    printf("click : '%s'\n", btn->user);
    buffer[idx++] = ((char *)btn->user)[0];
    buffer[idx] = 0;

    if (buffer[idx - 1] == '=')
    {
        parser_t parser;
        parser.buf = buffer;
        parser.size = idx;
        parser.id = 0;

        val_t v = parse_add(&parser);
        if (v.type == -1)
            return;
        if (v.type == 0)
            sprintf(result, "%i", v.as.i);
        else
            sprintf(result, "%f", v.as.i);

        label_set_text(display, result, 14);
        widget_update(&display->widget);

        idx = 0;
        result[0] = 0;
        buffer[idx] = 0;
        return;
    }

    label_set_text(display, buffer, 14);
    widget_update(&display->widget);
}

void clear(button_t *btn)
{
    printf("clear\n");
    idx = 0;
    result[0] = 0;
    buffer[idx] = 0;
    label_set_text(display, NULL, 14);
    widget_update(&display->widget);
}

button_t *place_btn(container_t *p, const char *text)
{
    button_t *b = new_button(&p->widget, text, (void *)text);
    widget_set_size(&b->widget, 30, 30);
    button_on_click(b, button_click, 0);
    return b;
}

void calculator(widget_t *root)
{
    display = new_label(root, "", "display");
    label_set_fontsize(display, 14);
    widget_set_size(&display->widget, 196, 30);

    container_t *raw3 = new_container(root, 1);
    widget_set_size(&raw3->widget, root->r.w, 40);

    place_btn(raw3, "7");
    place_btn(raw3, "8");
    place_btn(raw3, "9");
    place_btn(raw3, "/");
    button_t *b = place_btn(raw3, "C");
    b->on_click = clear;
    place_btn(raw3, " ");

    container_t *raw2 = new_container(root, 1);
    widget_set_size(&raw2->widget, root->r.w, 40);

    place_btn(raw2, "4");
    place_btn(raw2, "5");
    place_btn(raw2, "6");
    place_btn(raw2, "x");
    place_btn(raw2, " ");
    place_btn(raw2, " ");

    container_t *raw1 = new_container(root, 1);
    widget_set_size(&raw1->widget, root->r.w, 40);

    place_btn(raw1, "1");
    place_btn(raw1, "2");
    place_btn(raw1, "3");
    place_btn(raw1, "-");
    place_btn(raw1, "(");
    place_btn(raw1, ")");

    container_t *raw0 = new_container(root, 1);
    widget_set_size(&raw0->widget, root->r.w, 40);

    place_btn(raw0, "0");
    place_btn(raw0, ".");
    place_btn(raw0, "%");
    place_btn(raw0, "+");
    place_btn(raw0, "=");
    printf("raw0::%d\n", raw0->widget.childs->size);
    foreach (raw0->widget.childs, n)
    {
        widget_t *w = n->value;
        printf("*[%d,%d %dx%d]\n", w->r.x, w->r.y, w->r.w, w->r.h);
    }
}

int main(int argc, char const *argv[])
{
    app_t *app = new_application(argc, argv);
    if (!app)
    {
        printf("couldnt create app\n");
        return EXIT_FAILURE;
    }

    window_t *win = new_window(app, 200, 300, 0);
    if (!win)
    {
        printf("error create window\n");
        return EXIT_FAILURE;
    }
    window_set_backgound_color(win, rgba_color(250, 250, 250, 255));
    window_set_position(win, 450, 100);

    widget_set_layout(win->root, 2, 0);
    calculator(win->root);
    printf("====== show window =========\n");
    window_show(win);

    return app_main(app);
}
