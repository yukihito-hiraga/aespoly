#pragma once
#pragma GCC optimize("unroll-loops")
#include "../common/common.h"
#include <stddef.h>

typedef struct _aesgcm_context
{
	alignas(16) __m128i poly, poly_original;
	alignas(16) __m128i htbl[16];
	aes_context aesctx;
} aesgcm_context;