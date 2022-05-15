#include <mink/types.h>
#include <mink/debug.h>
#include <mink/i386.h>
#include <mink/stdio.h>
#include <ubsan.h>

#define LOCATION(l) printf("\nError: %s [%s: %d:%d] : \n", __func__, l.file_name, l.line, l.column)
#define DESCRIPTOR(d) printf("        * (%s, kind %x, info %x)\n", d->type_name, d->type_kind, d->type_info)

void __ubsan_handle_vla_bound_not_positive(struct vla_bound_data *data, unsigned long bound)
{
    CLI();
    LOCATION(data->location);
    DESCRIPTOR(data->type);
    printf("        * bound:%x\n", bound);

    stack_trace(10);
    HLT();
}

void __ubsan_handle_negate_overflow(struct overflow_data *data, uint32_t old_val)
{
    CLI();
    LOCATION(data->location);
    DESCRIPTOR(data->type);
    printf("        * old val:%x\n", old_val);

    stack_trace(10);
    HLT();
}
void __ubsan_handle_shift_out_of_bounds(struct shift_out_of_bounds_data *data, uint32_t lhs, uint32_t rhs)
{
    CLI();
    LOCATION(data->location);
    DESCRIPTOR(data->lhs_type);
    DESCRIPTOR(data->rhs_type);
    printf("    lhs:%d, rhs:%d\n", lhs, rhs);

    stack_trace(10);
    HLT();
}
void __ubsan_handle_sub_overflow(struct overflow_data *data, uint32_t lhs, uint32_t rhs)
{
    CLI();
    LOCATION(data->location);
    DESCRIPTOR(data->type);

    printf("    lhs:%x, rhs:%x\n", lhs, rhs);
    stack_trace(10);
    HLT();
}

void __ubsan_handle_add_overflow(struct overflow_data *data, uint32_t lhs, uint32_t rhs)
{
    CLI();
    LOCATION(data->location);
    DESCRIPTOR(data->type);
    printf("    lhs:%x, rhs:%x\n", lhs, rhs);
    stack_trace(10);
    HLT();
}

void __ubsan_handle_out_of_bounds(struct out_of_bounds_data *data, uint32_t idx)
{
    CLI();
    LOCATION(data->location);
    DESCRIPTOR(data->array_type);
    DESCRIPTOR(data->index_type);
    printf("    index:%d\n", idx);
    stack_trace(10);
    HLT();
}

void __ubsan_handle_divrem_overflow(struct overflow_data *data, uint32_t lhs, uint32_t rhs)
{
    CLI();
    LOCATION(data->location);
    DESCRIPTOR(data->type);
    printf("    lhs:%x, rhs:%x\n", lhs, rhs);
    stack_trace(10);
    HLT();
}

void __ubsan_handle_mul_overflow(struct overflow_data *data, uint32_t lhs, uint32_t rhs)
{
    CLI();
    LOCATION(data->location);
    DESCRIPTOR(data->type);
    printf("    lhs:%x, rhs:%x\n", lhs, rhs);
    stack_trace(10);
    HLT();
}

void __ubsan_handle_type_mismatch_v1(struct type_mismatch_data_v1 *data, uint32_t ptr)
{
    CLI();
    LOCATION(data->location);
    DESCRIPTOR(data->type);

    printf("    ptr:%p [align:%d chkkind:%d]\n", ptr, data->log_alignment, data->type_check_kind);
    stack_trace(10);
    HLT();
}

void __ubsan_handle_pointer_overflow(struct pointer_overflow_data *data, uint32_t base, uint32_t result)
{
    CLI();
    LOCATION(data->location);
    printf("    base:%p result:%p\n", base, result);
    stack_trace(10);
    HLT();
}
