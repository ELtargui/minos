#include <mink/i386.h>

uint8_t inportb(uint16_t port)
{
    uint8_t data;
    asm volatile("in %1,%0"
                 : "=a"(data)
                 : "d"(port));
    return data;
}

void outportb(uint16_t port, uint8_t data)
{
    asm volatile("out %0,%1"
                 :
                 : "a"(data), "d"(port));
}

uint16_t inportw(uint16_t port)
{
    uint16_t data;
    asm volatile("inw %1,%0"
                 : "=a"(data)
                 : "d"(port));
    return data;
}

void outportw(uint16_t port, uint16_t data)
{
    asm volatile("outw %0,%1"
                 :
                 : "a"(data), "d"(port));
}

uint32_t inportd(uint16_t port)
{
    uint32_t data;
    asm volatile("inl %1,%0"
                 : "=a"(data)
                 : "d"(port));
    return data;
}

void outportd(uint16_t port, uint32_t data)
{
    asm volatile("outl %0,%1"
                 :
                 : "a"(data), "d"(port));
}
