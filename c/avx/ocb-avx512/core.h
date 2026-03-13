#pragma once
#include "../common/common.h"

#define max_num_block 10000

alignas(64) __m512i Lstbl_o[max_num_block];
alignas(64) __m512i Lstbl[max_num_block];
alignas(64) __m512i Lstbl2[max_num_block];

typedef struct _ocb_context
{
	size_t blocklength;
	alignas(64) __m512i poly;
	alignas(64) __m512i L_dollar, L_star;
	alignas(64) __m512i Ltbl_first;
	alignas(64) __m512i Ltbl[5];
	alignas(64) __m512i Lstbl[1];
	alignas(64) __m512i mpoly[4];
	alignas(64) __m512i omegapoly;
	alignas(64) __m512i omegafirst;
	alignas(64) __m512i two;
	alignas(64) __m512i shuffle;
	alignas(64) __m512i four;
	alignas(64) __m512i mask;
} ocb_context;
