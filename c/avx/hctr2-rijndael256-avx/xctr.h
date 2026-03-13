#pragma once
#include "../common/common.h"

#include "core.h"

static inline void xctrxoradd256(hctr2_context ctx, __m128i *S, uint8_t *M, size_t Mlen, uint8_t *C)
{
	alignas(16) __m128i tmps[32];
	alignas(16) __m128i data[32];

	size_t Mrem = Mlen % 32;
	size_t dlen = Mlen / 32;
	size_t dx16len = dlen / 16;
	size_t dx16rem = dlen % 16;

	alignas(16) __m128i ctr[32];
	for (size_t i = 0; i < 16; i++)
	{
		ctr[i * 2] = _mm_xor_si128(S[0], _mm_setr_epi32(0, 0, 0, 0));
		ctr[i * 2 + 1] = _mm_xor_si128(S[1], _mm_setr_epi32(i, 0, 0, 0));
	}
	alignas(16) __m128i inc = _mm_setr_epi32(1, 0, 0, 0);
	alignas(16) __m128i inc16 = _mm_setr_epi32(16, 0, 0, 0);

	for (size_t i = 0; i < dx16len; i++)
	{
		loadx32((__m128i *)M + i * 32, data);
		rijndael256x16(ctx.rijndael256ctx, ctr, tmps);
		xorx32_1wise(tmps, data, tmps);
		storex32((__m128i *)C + i * 32, tmps);
		addx16_bfix_2wise(ctr+1, inc16, ctr+1);
	}

	for (size_t i = 0; i < dx16rem; i++)
	{
		loadx2((__m128i *)M + dx16len * 32 + i * 2, data);
		rijndael256x1(ctx.rijndael256ctx, ctr, tmps);
		xorx2_1wise(tmps, data, tmps);
		storex2((__m128i *)C + dx16len * 32 + i * 2, tmps);
		addx1_bfix_2wise(ctr+1, inc, ctr+1);
	}

	if (Mrem > 0)
	{
		uint8_t padded[32];
		memset(padded, 0, 32);
		memcpy(padded, M + Mlen - Mrem, Mrem);
		alignas(16) __m128i lastblk[2];
		lastblk[0] = _mm_loadu_si128((__m128i *)padded);
		lastblk[1] = _mm_loadu_si128((__m128i *)padded + 1);

		rijndael256x16(ctx.rijndael256ctx, ctr, tmps);
		xorx2_1wise(lastblk, tmps, tmps);

		memcpy((__m128i *)(C + dlen * 32), (uint8_t *)tmps, Mrem);
	}
}