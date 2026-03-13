#pragma once
#include "../common/common.h"

#include "core.h"

static inline void gctr128x4(aesgcm_context ctx, __m128i S, uint8_t *M, size_t Mlen, uint8_t *C)
{
	alignas(16) __m128i tmps[4];
	alignas(16) __m128i data[4];

	size_t blen = Mlen / 16;
	size_t remainder = Mlen % 16;
	size_t bx4len = blen / 4;
	size_t bx4rem = blen % 4;

	alignas(16) __m128i ctr[4];
	for (size_t i = 0; i < 4; i++)
	{
		ctr[i] = _mm_add_epi16(S, _mm_setr_epi32(i, 0, 0, 0));
	}

	alignas(16) __m128i inc = _mm_setr_epi32(1, 0, 0, 0);

	for (size_t i = 0; i < bx4len; i++)
	{
		copyx4(ctr, tmps);
		aesx4(ctx.aesctx.keys128, tmps, tmps);
		loadx4((__m128i *)M + 4 * i, data);
		xorx4_1wise(tmps, data, tmps);
		storex4((__m128i *)C + 4 * i, tmps);
		addx4_bfix(ctr, inc, ctr);
	}

	for (size_t i = 0; i < bx4rem; i++)
	{
		tmps[0] = aesenc128(ctr[0], ctx.aesctx.keys128);
		data[0] = _mm_loadu_si128((__m128i *)M + bx4len * 4 + i);
		tmps[0] = _mm_xor_si128(tmps[0], data[0]);
		_mm_storeu_si128((__m128i *)C + 4 * i, tmps[0]);
		ctr[0] = _mm_add_epi64(ctr[0], inc);
	}

	if (remainder > 0)
	{
		uint8_t padded[16];
		memset(padded, 0, 16);
		memcpy(padded, M + Mlen - remainder, remainder);
		alignas(16) __m128i lastblk = _mm_loadu_si128(((__m128i *)padded));
		tmps[0] = aesenc128(_mm_xor_si128(S, _mm_setr_epi64(_mm_set_pi64x((blen + 1)), _mm_setzero_si64())), ctx.aesctx.keys128);
		tmps[0] = _mm_xor_si128(_mm_loadu_si128((__m128i *)&lastblk), tmps[0]);
		memcpy((__m128i *)(C + Mlen - remainder), (uint8_t *)(&tmps[0]), remainder);
	}
}

static inline void gctr128x8(aesgcm_context ctx, __m128i S, uint8_t *M, size_t Mlen, uint8_t *C)
{
	alignas(16) __m128i tmps[8];
	alignas(16) __m128i data[8];

	size_t blen = Mlen / 16;
	size_t remainder = Mlen % 16;
	size_t bx8len = blen / 8;
	size_t bx8rem = blen % 8;

	alignas(16) __m128i ctr[8];
	for (size_t i = 0; i < 8; i++)
	{
		ctr[i] = _mm_add_epi16(S, _mm_setr_epi32(i, 0, 0, 0));
	}

	alignas(16) __m128i inc = _mm_setr_epi32(1, 0, 0, 0);

	for (size_t i = 0; i < bx8len; i++)
	{
		copyx8(ctr, tmps);
		aesx8(ctx.aesctx.keys128, tmps, tmps);
		loadx8((__m128i *)M + 8 * i, data);
		xorx8_1wise(tmps, data, tmps);
		storex8((__m128i *)C + 8 * i, tmps);
		addx8_bfix(ctr, inc, ctr);
	}

	for (size_t i = 0; i < bx8rem; i++)
	{
		tmps[0] = aesenc128(ctr[0], ctx.aesctx.keys128);
		data[0] = _mm_loadu_si128((__m128i *)M + bx8len * 4 + i);
		tmps[0] = _mm_xor_si128(tmps[0], data[0]);
		_mm_storeu_si128((__m128i *)C + 8 * i, tmps[0]);
		ctr[0] = _mm_add_epi64(ctr[0], inc);
	}

	if (remainder > 0)
	{
		uint8_t padded[16];
		memset(padded, 0, 16);
		memcpy(padded, M + Mlen - remainder, remainder);
		alignas(16) __m128i lastblk = _mm_loadu_si128(((__m128i *)padded));
		tmps[0] = aesenc128(_mm_xor_si128(S, _mm_setr_epi64(_mm_set_pi64x((blen + 1)), _mm_setzero_si64())), ctx.aesctx.keys128);
		tmps[0] = _mm_xor_si128(_mm_loadu_si128((__m128i *)&lastblk), tmps[0]);
		memcpy((__m128i *)(C + Mlen - remainder), (uint8_t *)(&tmps[0]), remainder);
	}
}