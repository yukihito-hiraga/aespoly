#pragma once
#include "../common/common.h"
#include "../simpira-neon/simpira-b2.h"
#include <stdbool.h>

typedef struct _hctr2_context
{
	simpira_context simpira_ctx;
	alignas(16) __m128i key[2];
	alignas(16) __m128i poly;
	alignas(16) __m128i htbl[32];
	alignas(16) __m128i L[2];
} hctr2_context;