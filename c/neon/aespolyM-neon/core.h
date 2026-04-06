#pragma once
#pragma GCC optimize("unroll-loops")
#include "../common/common.h"
#include <stddef.h>


typedef struct _aespolyM_context
{
    aes_context aes_ctx;
    alignas(16) __m128i poly;
    alignas(16) __m128i htbl[16];
} aespolyM_context;
