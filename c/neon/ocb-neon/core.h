#pragma once
#include "../common/common.h"
#include "../common/graycode.h"

typedef struct _ocb_context
{
	aes_context aesctx;
	alignas(16) __m128i poly;
	alignas(16) __m128i pp;
	alignas(16) __m128i L[300];
	alignas(16) __m128i L_dollar, L_star;
} ocb_context;
