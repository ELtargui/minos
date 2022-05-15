#pragma once

typedef struct symbole
{
    char *name;
    unsigned int address;
} symbole_t;

symbole_t *symbole_from_address(unsigned int address);
void stack_trace(int maxframes);
void stack_trace_impl(int max, void *frame);
