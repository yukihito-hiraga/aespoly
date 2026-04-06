#pragma once
#include "../common/common.h"
#include "../simpira-avx/simpira-b2.h"
#include "../common/graycode.h"
#include <stdbool.h>

alignas(16) __m128i L[300];
alignas(16) __m128i omega[300];

typedef struct _aespolyW_context
{
	simpira_context simpira_ctx;
	alignas(16) __m128i key[2];
	alignas(16) __m128i poly, poly_double;
	alignas(16) __m128i htbl[32];
} aespolyW_context;
