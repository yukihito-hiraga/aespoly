#pragma once
#pragma GCC optimize("unroll-loops")
#include "../common/common.h"
#include <stddef.h>


typedef struct _pmac_context
{
    alignas(16) __m128i L[64];
    alignas(16) __m128i Linv;
    alignas(16) __m128i poly, invpoly, pp;
} pmac_context;