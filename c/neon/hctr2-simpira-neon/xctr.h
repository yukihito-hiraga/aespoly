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
	alignas(16) __m128i inc16[32];
	alignas(16) __m128i inc[32];
	for (size_t i = 0; i < 16; i++)
	{
		ctr[i * 2] = _mm_xor_si128(S[0], _mm_setr_epi32(0, 0, 0, 0));
		ctr[i * 2 + 1] = _mm_xor_si128(S[1], _mm_setr_epi32(i, 0, 0, 0));
		inc16[2*i] = _mm_srli_si128(_mm_setr_epi32(1 << 2, 0, 0, 0), 2);
		inc16[2*i+1] = _mm_setr_epi32(16, 0, 0, 0);
		inc[2*i] = _mm_srli_si128(_mm_setr_epi32(1 << 2, 0, 0, 0), 2);
		inc[2*i+1] = _mm_setr_epi32(1, 0, 0, 0);
	}

	for (size_t i = 0; i < dx16len; i++)
	{
		loadx32((__m128i *)M + i * 32, data);
		addkey256x16(ctx.simpira_ctx.keys, ctr, tmps);
		simpira_b2x16_128(ctx.simpira_ctx, tmps);
		addkey256x16(ctx.simpira_ctx.keys, tmps, tmps);
		xorx32_1wise(tmps, data, tmps);
		storex32((__m128i *)C + i * 32, tmps);
		addx32_1wise(inc16, ctr, ctr);
	}

	for (size_t i = 0; i < dx16rem; i++)
	{
		loadx2((__m128i *)M + dx16len * 32 + i * 2, data);
		addkey256(ctx.key, ctr, tmps);
		simpira_b2x1_128(ctx.simpira_ctx, tmps);
		addkey256(ctx.simpira_ctx.keys, tmps, tmps);
		xorx2_1wise(data, tmps, tmps);
		storex2((__m128i *)C + dx16len * 32 + i * 2, tmps);
		addx2_1wise(inc, ctr, ctr);
	}

	if (Mrem > 0)
	{
		uint8_t padded[32];
		memset(padded, 0, 32);
		memcpy(padded, M + Mlen - Mrem, Mrem);
		alignas(16) __m128i lastblk[2];
		lastblk[0] = _mm_loadu_si128((__m128i *)padded);
		lastblk[1] = _mm_loadu_si128((__m128i *)padded);

		copyx2(ctr, tmps);
		xorx2_1wise(ctx.key, tmps, tmps);
		simpira_b2_128(ctx.simpira_ctx, tmps[0], tmps[1]);
		xorx2_1wise(ctx.key, tmps, tmps);
		xorx2_1wise(lastblk, tmps, tmps);

		memcpy((__m128i *)(C + dlen * 32), (uint8_t *)tmps, Mrem);
	}
}