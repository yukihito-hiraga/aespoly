#pragma once
#pragma GCC optimize("unroll-loops")
#include "../common/common.h"
#include "../simpira-avx/simpira-b2.h"
#include <stddef.h>

#define Lsize 320000

alignas(16) __m128i L[Lsize];

typedef struct _eme_context
{
    alignas(16) __m128i pp[2];
    alignas(16) __m128i pp16_1[2];
    alignas(16) __m128i pp16_2[2];
    alignas(16) __m128i poly[2];

    simpira_context simpiractx;
    alignas(16) __m128i key[2];

} eme_context;