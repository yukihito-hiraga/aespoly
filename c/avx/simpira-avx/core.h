#pragma once
#include "../common/common.h"

typedef struct _simpira_context
{
    alignas(16) __m128i c[20];
    alignas(16) __m128i c2[20];
    alignas(16) __m128i keys[2];
} simpira_context;