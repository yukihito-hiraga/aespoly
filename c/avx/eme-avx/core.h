#pragma once
#pragma GCC optimize("unroll-loops")
#include "../common/common.h"
#include <stddef.h>

#define Lsize 320000

alignas(16) __m128i L[Lsize];

typedef struct _eme_context
{
    alignas(16) __m128i pp, pp16_1, pp16_2, poly;
    aes_context aesctx;
} eme_context;