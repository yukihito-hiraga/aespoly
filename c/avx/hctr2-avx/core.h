#pragma once
#include "../common/common.h"
#include <stddef.h>


typedef struct _hctr2_context
{
	size_t blocklength;
	alignas(16) __m128i poly;
	alignas(16) __m128i htbl[16];
	alignas(16) __m128i L;
} hctr2_context;