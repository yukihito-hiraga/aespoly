#pragma once
#pragma GCC optimize("unroll-loops")
#include "../common/common.h"
#include <stddef.h>


typedef struct _ghash_poly_context
{
    alignas(16) __m128i poly;
    alignas(16) __m128i htbl[16];
} ghash_poly_context;