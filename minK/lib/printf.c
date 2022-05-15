#include <mink/types.h>
#include <mink/stdio.h>

void *std_outstream = NULL;
stdout_putc_t std_putc;

void set_std_output(void *stream, stdout_putc_t putc)
{
    std_outstream = stream;
    std_putc = putc;
}

int printf(const char *fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    int c = sformat(std_putc, std_outstream, fmt, vl);
    va_end(vl);
    return c;
}

struct sprintf_ctx
{
    char *buf;
    int size;
    int index;
};

static int sprintf_putc(void *stream, int c)
{
    struct sprintf_ctx *ctx = stream;
    if (ctx->index >= ctx->size)
        return 0;
    ctx->buf[ctx->index++] = c;
    ctx->buf[ctx->index] = 0;
    return 1;
}

int sprintf(char *buf, const char *fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    struct sprintf_ctx ctx;
    ctx.buf = buf;
    ctx.index = 0;
    ctx.size = INTMAX;

    int c = sformat(sprintf_putc, &ctx, fmt, vl);
    va_end(vl);
    return c;
}

#define PUT(c)            \
    if (!putc(stream, c)) \
        return cnt;       \
    ++cnt

static int put_number(int (*putc)(void *, int), void *stream, int base, int _signed, uint32_t number)
{
    static char *digits = "0123456789abcdef";

    int cnt = 0;

    int digits_cnt = 1;
    char s[30];

    if (base == 10 && _signed)
    {
        int n = (int32_t)number;
        if (n < 0)
        {
            PUT('-');
            number = (uint32_t)-n;
        }
    }

    uint32_t copy = number;
    while ((uint32_t)base <= copy)
    {
        copy /= base;
        digits_cnt++;
    }

    if (base == 16 && _signed)
    {
        PUT('0');
        PUT('x');
        int z = 8 - digits_cnt;
        while (z > 0)
        {
            PUT('0');
            z--;
        }
    }

    s[digits_cnt] = 0;
    for (int i = digits_cnt; i != 0; i--)
    {
        s[i - 1] = digits[number % base];
        number /= base;
    }

    char *sp = s;
    while (*sp)
    {
        PUT(*sp++);
    }
    return cnt;
}

int sformat(stdout_putc_t putc, void *stream, const char *fmt, va_list vl)
{
    int cnt = 0;
    while (*fmt)
    {
        if (*fmt == '%')
        {
            fmt++;
            switch (*fmt)
            {
            case 'd':
                cnt += put_number(putc, stream, 10, 1, va_arg(vl, uint32_t));
                break;
            case 'i':
                cnt += put_number(putc, stream, 10, 0, va_arg(vl, int));
                break;
            case 'p':
                cnt += put_number(putc, stream, 16, 1, va_arg(vl, uint32_t));
                break;
            case 'x':
                cnt += put_number(putc, stream, 16, 0, va_arg(vl, uint32_t));
                break;
            case 'c':
                PUT(va_arg(vl, int));
                break;
            case 's':
            {

                char *s = (char *)va_arg(vl, char *);
                if (s == NULL)
                    s = "(null)";

                while (*s)
                {
                    PUT(*s++);
                }
                break;
            }
            case 0:
                return cnt;
            default:
                PUT(*fmt);
                continue;
            }
            fmt++;
        }
        else
        {
            PUT(*fmt++);
        }
    }
    return cnt;
}
