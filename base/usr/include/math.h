#pragma once 


#define HUGE_VALF __builtin_huge_valf()
#define HUGE_VAL __builtin_huge_val()
#define HUGE_VALL __builtin_huge_vall()

#define INFINITY __builtin_huge_valf()
#define INF __builtin_huge_val()

#define NAN __builtin_nan("")
#define MAXFLOAT FLT_MAX

#define isnan(x) __builtin_isnan(x)
#define isinf(x) __builtin_isinf_sign(x)

