#pragma once
#include "../common/common.h"
#include "../rijndael256/rijndael256-avx.h"
#include <stdbool.h>

typedef struct _hctr2_context
{
	rijndael256_context rijndael256ctx;
	alignas(16) __m128i poly;
	alignas(16) __m128i htbl[32];
	alignas(16) __m128i L[20];
} hctr2_context;