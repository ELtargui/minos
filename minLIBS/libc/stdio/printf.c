#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

enum fmt_flags
{
    FLAGS_NONE = 0,
    LEFT_JUSTIFIED,   // -
    SIGNED,           // +
    SPACE,            // ' '
    ALTERNATIVE_FORM, // #
    LEADING_ZEROS,    // 0
};

enum length_modifiers
{
    LENGTH_none = 0,
    LENGTH_hh,
    LENGTH_h,
    LENGTH_l,
    LENGTH_ll,
    LENGTH_j,
    LENGTH_z,
    LENGTH_t,
    LENGTH_L,
};

#define PUT(x)              \
    if (put(x, stream) < 0) \
        return -1;          \
    cnt++

static int number_tostring(char *out, uint64_t value, unsigned int base, char *digits)
{
    int result = 1;
    uint64_t copy = value;
    while (base <= copy)
    {
        copy /= base;
        result++;
    }
    out[result] = 0;
    for (int i = result; i != 0; i--)
    {
        out[i - 1] = digits[value % base];
        value /= base;
    }

    return result;
}

int format_string(int (*put)(int, FILE *), void *stream, const char *fmt, va_list ap)
{
    int cnt = 0;
    char *s = (char *)fmt;
    while (*s)
    {
        if (*s == '%')
        {
            s++;
            int flag_left_justified = 0;
            int flag_begin_with_a_sign = 0;
            int flag_alternative_form = 0;
            int flag_space = 0;
            int flag_leading_zeros = 0;
            while (1)
            {
                int found = 0;
                switch (*s)
                {
                case '-':
                    flag_left_justified++;
                    found++;
                    break;
                case '+':
                    flag_begin_with_a_sign++;
                    found++;
                    break;
                case '#':
                    flag_alternative_form++;
                    found++;
                    break;
                case ' ':
                    flag_space++;
                    found++;
                    break;
                case '0':
                    flag_leading_zeros++;
                    found++;
                    break;
                }
                if (!found)
                    break;
                s++;
            }
            int field_width = 0;
            if (*s == '*')
            {
                s++;
                field_width = va_arg(ap, int);
            }
            else
            {
                while (isdigit(*s))
                {
                    field_width = field_width * 10 + *s++ - '0';
                }
            }
            int precision = 0;
            if (*s == '.')
            {
                s++;
                if (*s == '*')
                {
                    s++;
                    precision = va_arg(ap, int);
                }
                else
                {
                    while (isdigit(*s))
                    {
                        precision = precision * 10 + *s++ - '0';
                    }
                }
            }
            int length_modifier = LENGTH_none;
            switch (*s)
            {
            case 'h':
                if (s[1] == 'h')
                {
                    length_modifier = LENGTH_hh;
                    s++;
                }
                else
                    length_modifier = LENGTH_h;
                s++;
                break;
            case 'l':
                if (s[1] == 'l')
                {
                    length_modifier = LENGTH_ll;
                    s++;
                }
                else
                    length_modifier = LENGTH_l;
                s++;
                break;
            case 'j':
                length_modifier = LENGTH_j;
                s++;
                break;
            case 'z':
                length_modifier = LENGTH_z;
                s++;
                break;
            case 't':
                length_modifier = LENGTH_t;
                s++;
                break;
            case 'L':
                length_modifier = LENGTH_L;
                s++;
                break;
            }

            switch (*s)
            {
            case 'i':
            case 'd':
            case 'o':
            case 'x':
            case 'X':
            case 'p':
            {
                uintmax_t number = 0;
                int is_signed = 0;
                if (*s == 'i' || *s == 'd')
                {
                    is_signed = 1;

                    switch (length_modifier)
                    {
                    case LENGTH_none:
                        number = va_arg(ap, signed int);
                        break;
                    case LENGTH_hh:
                        number = 0xff & va_arg(ap, signed int);
                        break;
                    case LENGTH_h:
                        number = 0xffff & va_arg(ap, signed int);
                        break;
                    case LENGTH_l:
                        number = va_arg(ap, signed long);
                        break;
                    case LENGTH_ll:
                        number = va_arg(ap, signed long long);
                        break;
                    case LENGTH_j:
                        number = va_arg(ap, intmax_t);
                        break;
                    case LENGTH_z:
                        number = va_arg(ap, ssize_t);
                        break;
                    case LENGTH_t:
                        number = va_arg(ap, ptrdiff_t);
                        break;
                    case LENGTH_L:
                        number = va_arg(ap, long double);
                        break;
                    }
                }
                else
                {
                    switch (length_modifier)
                    {
                    case LENGTH_none:
                        number = va_arg(ap, unsigned int);
                        break;
                    case LENGTH_hh:
                        number = (uint8_t)va_arg(ap, int);
                        break;
                    case LENGTH_h:
                        number = (uint16_t)va_arg(ap, int);
                        break;
                    case LENGTH_l:
                        number = va_arg(ap, unsigned long);
                        break;
                    case LENGTH_ll:
                        number = va_arg(ap, unsigned long long);
                        break;
                    case LENGTH_j:
                        number = va_arg(ap, uintmax_t);
                        break;
                    case LENGTH_z:
                        number = va_arg(ap, size_t);
                        break;
                    case LENGTH_t:
                        number = va_arg(ap, ptrdiff_t);
                        break;
                    case LENGTH_L:
                        number = va_arg(ap, long double);
                        break;
                    }
                }
                char *digits;
                int base = 0;
                char tmp[64];

                if (*s == 'i' || *s == 'd')
                {
                    base = 10;
                    digits = "0123456789";
                }
                else if (*s == 'o')
                {
                    flag_begin_with_a_sign = 0;
                    base = 8;
                    digits = "01234567";
                }
                else
                {
                    base = 16;
                    flag_begin_with_a_sign = 0;
                    if (*s == 'X')
                        digits = "0123456789ABCDEF";
                    else
                        digits = "0123456789abcdef";
                    if (*s == 'p')
                        flag_alternative_form = 1;
                }
                int is_negative = 0;
                if (is_signed)
                {
                    flag_alternative_form = 0;
                    intmax_t n = (intmax_t)number;
                    if (n < 0)
                    {
                        is_negative = 1;
                        number = (uintmax_t)-n;
                    }
                }
                int len = number_tostring(tmp, number, base, digits);

                if (is_negative)
                {
                    PUT('-');
                }
                else if (flag_begin_with_a_sign)
                {
                    PUT('+');
                }
                else if (flag_space)
                {
                    PUT(' ');
                }

                if (flag_alternative_form)
                {
                    PUT('0');

                    if (base == 16)
                    {
                        if (*s == 'X')
                        {
                            PUT('X');
                        }
                        else
                        {
                            PUT('x');
                        }
                    }
                }
                int width = len;
                if (len < precision)
                {
                    width = precision;
                }
                if (!flag_left_justified)
                {
                    if (width < field_width)
                    {
                        if (flag_leading_zeros)
                            for (int i = 0; i < field_width - width; i++)
                            {
                                PUT('0');
                            }
                        else
                            for (int i = 0; i < field_width - width; i++)
                            {
                                PUT(' ');
                            }
                    }
                }

                for (int i = 0; i < width - len; i++)
                {
                    PUT('0');
                }
                for (int i = 0; i < len; i++)
                {
                    PUT(tmp[i]);
                }

                if (flag_left_justified)
                {
                    if (width < field_width)
                    {
                        for (int i = 0; i < field_width - width; i++)
                        {
                            PUT(' ');
                        }
                    }
                }
            }
                s++;
                break;
            case 's':
            {
                s++;
                char *str = va_arg(ap, char *);
                if (!str)
                    str = "(null)";
                int len = 0;
                int width = 0;

                if (precision)
                {
                    len = precision;
                }
                else
                {
                    len = strlen(str);
                }
                {
                    int l = strlen(str);
                    if (l < len)
                        len = l;
                }
                if (!field_width)
                {
                    width = len;
                }
                else
                {
                    if (field_width < precision)
                        width = precision;
                    else
                        width = field_width;
                }
                if (!flag_left_justified)
                {
                    for (int i = 0; i < width - len; i++)
                    {
                        PUT(' ');
                    }
                }
                for (int i = 0; i < len; i++)
                {
                    if (str[i] == 0)
                        break;
                    PUT(str[i]);
                }
                if (flag_left_justified)
                {
                    for (int i = len; i < width; i++)
                    {
                        PUT(' ');
                    }
                }
            }
            break;

            case 'c':
            {
                s++;
                char b[1];
                b[0] = va_arg(ap, int);
                if (!field_width)
                    field_width = 1;
                if (!flag_left_justified)
                {
                    for (int i = 0; i < field_width - 1; i++)
                    {
                        PUT(' ');
                    }
                }
                PUT(b[0]);
                if (flag_left_justified)
                {
                    for (int i = 1; i < field_width; i++)
                    {
                        PUT(' ');
                    }
                }
                break;
            }
            case 'G':
            case 'F':
            case 'g': /* supposed to also support e */
            case 'f':
            {
                s++;
                if (!precision)
                    precision = 6;
                char out[128];
                int len = 0;
                double number = va_arg(ap, double);
                int is_negative = 0;
                // if (number == +INFINITY || number == -INFINITY)
                if (isinf(number))
                {
                    is_negative = number == -INFINITY;
                    out[len++] = 'i';
                    out[len++] = 'n';
                    out[len++] = 'f';
                }
                else if (isnan(number))
                {
                    out[len++] = 'n';
                    out[len++] = 'a';
                    out[len++] = 'n';
                }
                else
                {
                    is_negative = number < 0.0;
                    if (is_negative)
                        number = -number;
                    len = number_tostring(out, (uint64_t)number, 10, "0123456789");
                    out[len++] = '.';
                    double fraction = number - (uint64_t)number;
                    for (int i = 0; i < precision; i++)
                    {
                        fraction = fraction * 10;
                    }
                    len += number_tostring(out + len, (int64_t)fraction, 10, "0123456789");
                }
                //****************************************
                if (is_negative)
                {
                    PUT('-');
                }
                else if (flag_begin_with_a_sign)
                {
                    PUT('+');
                }
                else if (flag_space)
                {
                    PUT(' ');
                }

                // if (flag_alternative_form)
                // {

                // }

                int width = len;

                if (!flag_left_justified)
                {
                    if (width < field_width)
                    {
                        if (flag_leading_zeros)
                            for (int i = 0; i < field_width - width; i++)
                            {
                                PUT('0');
                            }
                        else
                            for (int i = 0; i < field_width - width; i++)
                            {
                                PUT(' ');
                            }
                    }
                }

                for (int i = 0; i < width - len; i++)
                {
                    PUT('0');
                }
                for (int i = 0; i < len; i++)
                {
                    PUT(out[i]);
                }

                if (flag_left_justified)
                {
                    if (width < field_width)
                    {
                        for (int i = 0; i < field_width - width; i++)
                        {
                            PUT(' ');
                        }
                    }
                }
                //****************************************
                break;
            }
            default:
                break;
            }
        }
        else
        {
            PUT(*s++);
        }
    }

    return cnt;
}

int vdprintf(int fd, const char *format, va_list ap)
{
    static FILE f;
    memset(&f, 0, sizeof(FILE));
    f.fd = fd;
    f.bufsize = 0;
    f.buffer_type = _IONBF;
    return format_string(fputc, &f, format, ap);
}

int vfprintf(FILE *stream, const char *format, va_list ap)
{
    return format_string(fputc, stream, format, ap);
}

int vprintf(const char *format, va_list ap)
{
    return format_string(putc_unlocked, stdout, format, ap);
}

struct vsnbuf
{
    char *s;
    int size;
    int index;
};

static int vsnprintf_put(int c, FILE *b)
{
    struct vsnbuf *buff = (struct vsnbuf *)b;
    if (buff->index >= buff->size)
        return EOF;
    buff->s[buff->index++] = c;
    return c;
}

int vsnprintf(char *s, size_t n, const char *format, va_list ap)
{
    static struct vsnbuf buf;
    buf.s = s;
    buf.size = (int)n;
    buf.index = 0;
    int r = format_string(vsnprintf_put, &buf, format, ap);
    if (buf.index == buf.size)
        s[n - 1] = 0;
    else
        s[buf.index] = 0;
    return r < buf.size ? r : buf.index;
}

int vsprintf(char *s, const char *format, va_list ap)
{
    struct vsnbuf buf;
    buf.s = s;
    buf.size = 0x100000;
    buf.index = 0;
    int r = format_string(vsnprintf_put, &buf, format, ap);
    s[buf.index] = 0;
    return r < buf.size ? r : buf.index;
}

int dprintf(int fd, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    int ret = vdprintf(fd, format, ap);
    va_end(ap);
    return ret;
}

int fprintf(FILE *stream, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    int ret = vfprintf(stream, format, ap);
    va_end(ap);
    return ret;
}

int printf(const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    int ret = vprintf(format, ap);
    va_end(ap);
    return ret;
}

int sprintf(char *s, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    int ret = vsprintf(s, format, ap);
    va_end(ap);
    return ret;
}

int snprintf(char *s, size_t n, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    int ret = vsnprintf(s, n, format, ap);
    va_end(ap);
    return ret;
}
