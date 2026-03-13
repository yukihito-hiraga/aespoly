#pragma once
#include "../common/common.h"
#include "../common/graycode.h"
#include <stdbool.h>


typedef struct _aespoly_context
{
	aes_context aes_ctx;
	alignas(16) __m128i poly, poly_double;
	alignas(16) __m128i htbl[16];
	alignas(16) __m128i L[300];
	alignas(16) __m128i omega[300];
} aespoly_context;
