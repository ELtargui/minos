#pragma once


struct source_location
{
    const char *file_name;
    unsigned int line;
    unsigned int column;
};

struct type_descriptor
{
    unsigned short type_kind;
    unsigned short type_info;
    char type_name[];
};

struct type_mismatch_data
{
    struct source_location location;
    struct type_descriptor *type;
    unsigned int alignment;
    unsigned char type_check_kind;
};

struct type_mismatch_data_v1
{
    struct source_location location;
    struct type_descriptor *type;
    unsigned char log_alignment;
    unsigned char type_check_kind;
};

struct overflow_data
{
    struct source_location location;
    struct type_descriptor *type;
};

struct shift_out_of_bounds_data
{
    struct source_location location;
    struct type_descriptor *lhs_type;
    struct type_descriptor *rhs_type;
};

struct out_of_bounds_data
{
    struct source_location location;
    struct type_descriptor *array_type;
    struct type_descriptor *index_type;
};

struct unreachable_data
{
    struct source_location location;
};

struct vla_bound_data
{
    struct source_location location;
    struct type_descriptor *type;
};

struct invalid_value_data
{
    struct source_location location;
    struct type_descriptor *type;
};

struct nonnull_arg_data
{
    struct source_location location;
};

struct nonnull_return_data
{
    struct source_location location;
    struct source_location attr_loc;
};

struct pointer_overflow_data
{
    struct source_location location;
};
