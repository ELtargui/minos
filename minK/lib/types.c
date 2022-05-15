#include <mink/types.h>
#include <mink/debug.h>

int to_digit(int d)
{
    assert(d >= '0' && d <= '9');
    return d - '0';
}

int to_xdigit(int x)
{
    if (x >= '0' && x <= '9')
        return x - '0';
    assert(x >= 'a' && x <= 'f');
    return 10 + (x - 'a');
}

int atoi(const char *s)
{
    assert(s);
    if (s[0] == '0' && s[1] == 'x')
    {
        s++;
        s++;

        uint32_t x = 0;
        while (*s)
        {
            x = (x << 4) | to_xdigit(*s++);
        }

        return (int)x;
    }
    else
    {
        int n = 0;
        while (*s)
        {
            if (*s < '0' && *s > '9')
                break;
            n = n * 10 + to_digit(*s++);
        }
        return n;
    }
}
