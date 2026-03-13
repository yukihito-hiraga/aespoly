#pragma once
#include "../common/common.h"
#include "core.h"
#include "hash.h"

#define xctrhash_round(ctx, ctr, tmps, ttmps, n, i)                                               \
	{                                                                                        \
		addkey256(ctx.simpira_ctx.keys, ctr + 2 * (n - 1 - i), tmps + 2 * (n - 1 - i));      \
		simpira_b2x1_128(ctx.simpira_ctx, tmps + 2 * (n - 1 - i));                           \
		addkey256(ctx.simpira_ctx.keys, tmps + 2 * (n - 1 - i), tmps + 2 * (n - 1 - i));     \
		xorx2_1wise(tmps + 2 * (n - 1 - i), data + 2 * (n - 1 - i), tmps + 2 * (n - 1 - i)); \
		muladd_n2(tmps, ctx.htbl, ttmps, 2 * n, 2 * i);                                      \
		muladd_n2(tmps, ctx.htbl, ttmps, 2 * n, 2 * i + 1);                                  \
	}


static inline void xctrxoradd_hash128x4(hctr2_context ctx, __m128i *state, __m128i *S, uint8_t *M, size_t Mlen, uint8_t *C, __m128i *hash)
{

	alignas(16) __m128i X[2] = {state[0], state[1]};
	alignas(16) __m128i Y[2] = {_mm_setzero_si128(), _mm_setzero_si128()};
	alignas(16) __m128i Z[2] = {_mm_setzero_si128(), _mm_setzero_si128()};

	size_t blen = Mlen / 16;
	size_t dlen = blen / 2;
	size_t dx4len = dlen / 4;
	size_t dx4rem = dlen % 4;

	alignas(16) __m128i data[16];
	alignas(16) __m128i tmps[24];

	alignas(16) __m128i ctr[16];
	alignas(16) __m128i inc4[16];
	alignas(16) __m128i inc[16];

	for (size_t i = 0; i < 4; i++)
	{
		ctr[2 * i] = _mm_setr_epi32(0, 0, 0, 0);
		ctr[2 * i + 1] = _mm_setr_epi32(i + 1, 0, 0, 0);
		inc4[2 * i] = _mm_srli_si128(_mm_setr_epi32(1 << 2, 0, 0, 0), 2);
		inc4[2 * i + 1] = _mm_setr_epi32(4, 0, 0, 0);
		inc[2 * i] = _mm_srli_si128(_mm_setr_epi32(1 << 2, 0, 0, 0), 2);
		inc[2 * i + 1] = _mm_setr_epi32(1, 0, 0, 0);
	}

	__m128i *ttmps = tmps + 16;

	for (size_t i = 0; i < dx4len; i++)
	{
		loadx8((__m128i *)M + 8 * i, data);

		Y[0] = polyreduce128(ctx.poly, Y[0]);
		Y[1] = polyreduce128(ctx.poly, Y[1]);
		Z[0] = _mm_xor_si128(X[0], Y[0]);
		Z[1] = _mm_xor_si128(X[1], Y[1]);
		
		///*
		addkey256x4(S, ctr, tmps);
		addkey256x4(ctx.simpira_ctx.keys, tmps, tmps);
		simpira_b2x4_128(ctx.simpira_ctx, tmps);
		addkey256x4(ctx.simpira_ctx.keys, tmps, tmps);
		xorx8_1wise(tmps, data, tmps);

		Z[0] = _mm_xor_si128(Z[0], tmps[0]);
		Z[1] = _mm_xor_si128(Z[1], tmps[0]);

		mulinit_n2(tmps, ctx.htbl, ttmps, 8);
		muladd_n2(tmps, ctx.htbl, ttmps, 8, 1);
		muladd_n2(tmps, ctx.htbl, ttmps, 8, 2);
		muladd_n2(tmps, ctx.htbl, ttmps, 8, 3);
		muladd_n2(tmps, ctx.htbl, ttmps, 8, 4);
		muladd_n2(tmps, ctx.htbl, ttmps, 8, 5);
		muladd_n2(tmps, ctx.htbl, ttmps, 8, 6);
		muladdlast_n2(Z, ctx.htbl, ttmps, 8);

		ttmps[3] = _mm_bsrli_si128(ttmps[2], 8);
		ttmps[2] = _mm_bslli_si128(ttmps[2], 8);

		ttmps[4 + 3] = _mm_bsrli_si128(ttmps[4 + 2], 8);
		ttmps[4 + 2] = _mm_bslli_si128(ttmps[4 + 2], 8);

		X[0] = _mm_xor_si128(ttmps[3], ttmps[1]);
		Y[0] = _mm_xor_si128(ttmps[0], ttmps[2]);

		X[1] = _mm_xor_si128(ttmps[4 + 3], ttmps[4 + 1]);
		Y[1] = _mm_xor_si128(ttmps[4 + 0], ttmps[4 + 2]);

		addx8_1wise(inc4, ctr, ctr);
		storex8((__m128i *)C + i * 8, tmps);
	}

	Y[0] = polyreduce128(ctx.poly, Y[0]);
	Z[0] = _mm_xor_si128(X[0], Y[0]);
	X[0] = Z[0];

	Y[1] = polyreduce128(ctx.poly, Y[1]);
	Z[1] = _mm_xor_si128(X[1], Y[1]);
	X[1] = Z[1];

	for (size_t i = 0; i < dx4rem; i++)
	{
		loadx2((__m128i *)M + dx4len * 8 + i * 2, data);
		copyx2(ctr, tmps);
		addkey256(ctx.key, ctr, tmps);
		simpira_b2_128(ctx.simpira_ctx, tmps[0], tmps[1]);
		addkey256(ctx.key, tmps, tmps);
		xorx2_1wise(tmps, data, tmps);

		X[0] = _mm_xor_si128(X[0], tmps[0]);
		X[0] = polydot128(ctx.poly, X[0], ctx.htbl[0]);
		X[0] = _mm_xor_si128(X[0], tmps[1]);
		X[0] = polydot128(ctx.poly, X[0], ctx.htbl[0]);

		X[1] = _mm_xor_si128(X[1], tmps[0]);
		X[1] = polydot128(ctx.poly, X[1], ctx.htbl[16]);
		X[1] = _mm_xor_si128(X[1], tmps[1]);
		X[1] = polydot128(ctx.poly, X[1], ctx.htbl[16]);

		addx2_1wise(inc, ctr, ctr);
		scatter_store_n2x1((__m128i *)C + dx4len * 8 + i * 2, tmps);
	}

	size_t Mrem = Mlen % 32;

	if (Mrem > 0)
	{
		uint8_t padded[32];
		memset(padded, 0, 32);
		memcpy(padded, M + Mlen - Mrem, Mrem);
		alignas(16) __m128i lastblk[2];
		lastblk[0] = _mm_loadu_si128((__m128i *)padded);
		lastblk[1] = _mm_loadu_si128((__m128i *)padded + 1);

		copyx2(ctr, tmps);
		xorx2_1wise(ctx.key, tmps, tmps);
		simpira_b2_128(ctx.simpira_ctx, tmps[0], tmps[1]);
		xorx2_1wise(ctx.key, tmps, tmps);
		xorx2_1wise(lastblk, tmps, tmps);

		memcpy((__m128i *)(C + dlen * 32), (uint8_t *)tmps, Mrem);

		((uint8_t *)&tmps)[Mrem] = 0x01;

		X[0] = _mm_xor_si128(X[0], tmps[0]);
		X[0] = polydot128(ctx.poly, X[0], ctx.htbl[0]);
		X[0] = _mm_xor_si128(X[0], tmps[1]);
		X[0] = polydot128(ctx.poly, X[0], ctx.htbl[0]);

		X[1] = _mm_xor_si128(X[1], tmps[0]);
		X[1] = polydot128(ctx.poly, X[1], ctx.htbl[16]);
		X[1] = _mm_xor_si128(X[1], tmps[1]);
		X[1] = polydot128(ctx.poly, X[1], ctx.htbl[16]);
	}

	hash[0] = X[0];
	hash[1] = X[1];
}


static inline void xctrxoradd_hash128x8(hctr2_context ctx, __m128i *state, __m128i *S, uint8_t *M, size_t Mlen, uint8_t *C, __m128i *hash)
{

	alignas(16) __m128i X[2] = {state[0], state[1]};
	alignas(16) __m128i Y[2] = {_mm_setzero_si128(), _mm_setzero_si128()};
	alignas(16) __m128i Z[2] = {_mm_setzero_si128(), _mm_setzero_si128()};

	size_t blen = Mlen / 16;
	size_t dlen = blen / 2;
	size_t dx8len = dlen / 8;
	size_t dx8rem = dlen % 8;

	alignas(16) __m128i data[16];
	alignas(16) __m128i tmps[24];

	alignas(16) __m128i ctr[16];
	alignas(16) __m128i inc8[16];
	alignas(16) __m128i inc[16];

	for (size_t i = 0; i < 8; i++)
	{
		ctr[2 * i] = _mm_setr_epi32(0, 0, 0, 0);
		ctr[2 * i + 1] = _mm_setr_epi32(i + 1, 0, 0, 0);
		inc8[2 * i] = _mm_srli_si128(_mm_setr_epi32(1 << 2, 0, 0, 0), 2);
		inc8[2 * i + 1] = _mm_setr_epi32(8, 0, 0, 0);
		inc[2 * i] = _mm_srli_si128(_mm_setr_epi32(1 << 2, 0, 0, 0), 2);
		inc[2 * i + 1] = _mm_setr_epi32(1, 0, 0, 0);
	}

	__m128i *ttmps = tmps + 16;

	for (size_t i = 0; i < dx8len; i++)
	{
		loadx16((__m128i *)M + 16 * i, data);

		Y[0] = polyreduce128(ctx.poly, Y[0]);
		Y[1] = polyreduce128(ctx.poly, Y[1]);
		Z[0] = _mm_xor_si128(X[0], Y[0]);
		Z[1] = _mm_xor_si128(X[1], Y[1]);
		
		///*
		addkey256x8(S, ctr, tmps);
		addkey256x8(ctx.simpira_ctx.keys, tmps, tmps);
		simpira_b2x8_128(ctx.simpira_ctx, tmps);
		addkey256x8(ctx.simpira_ctx.keys, tmps, tmps);
		xorx16_1wise(tmps, data, tmps);

		Z[0] = _mm_xor_si128(Z[0], tmps[0]);
		Z[1] = _mm_xor_si128(Z[1], tmps[0]);

		mulinit_n2(tmps, ctx.htbl, ttmps, 16);
		muladd_n2(tmps, ctx.htbl, ttmps, 16, 1);
		muladd_n2(tmps, ctx.htbl, ttmps, 16, 2);
		muladd_n2(tmps, ctx.htbl, ttmps, 16, 3);
		muladd_n2(tmps, ctx.htbl, ttmps, 16, 4);
		muladd_n2(tmps, ctx.htbl, ttmps, 16, 5);
		muladd_n2(tmps, ctx.htbl, ttmps, 16, 6);
		muladd_n2(tmps, ctx.htbl, ttmps, 16, 7);
		muladd_n2(tmps, ctx.htbl, ttmps, 16, 8);
		muladd_n2(tmps, ctx.htbl, ttmps, 16, 9);
		muladd_n2(tmps, ctx.htbl, ttmps, 16, 10);
		muladd_n2(tmps, ctx.htbl, ttmps, 16, 11);
		muladd_n2(tmps, ctx.htbl, ttmps, 16, 12);
		muladd_n2(tmps, ctx.htbl, ttmps, 16, 13);
		muladd_n2(tmps, ctx.htbl, ttmps, 16, 14);
		muladdlast_n2(Z, ctx.htbl, ttmps, 16);

		ttmps[3] = _mm_bsrli_si128(ttmps[2], 8);
		ttmps[2] = _mm_bslli_si128(ttmps[2], 8);

		ttmps[4 + 3] = _mm_bsrli_si128(ttmps[4 + 2], 8);
		ttmps[4 + 2] = _mm_bslli_si128(ttmps[4 + 2], 8);

		X[0] = _mm_xor_si128(ttmps[3], ttmps[1]);
		Y[0] = _mm_xor_si128(ttmps[0], ttmps[2]);

		X[1] = _mm_xor_si128(ttmps[4 + 3], ttmps[4 + 1]);
		Y[1] = _mm_xor_si128(ttmps[4 + 0], ttmps[4 + 2]);

		addx16_1wise(inc8, ctr, ctr);
		storex16((__m128i *)C + i * 16, tmps);
	}

	Y[0] = polyreduce128(ctx.poly, Y[0]);
	Z[0] = _mm_xor_si128(X[0], Y[0]);
	X[0] = Z[0];

	Y[1] = polyreduce128(ctx.poly, Y[1]);
	Z[1] = _mm_xor_si128(X[1], Y[1]);
	X[1] = Z[1];

	for (size_t i = 0; i < dx8rem; i++)
	{
		loadx2((__m128i *)M + dx8len * 16 + i * 2, data);
		copyx2(ctr, tmps);
		addkey256(ctx.key, ctr, tmps);
		simpira_b2_128(ctx.simpira_ctx, tmps[0], tmps[1]);
		addkey256(ctx.key, tmps, tmps);
		xorx2_1wise(tmps, data, tmps);

		X[0] = _mm_xor_si128(X[0], tmps[0]);
		X[0] = polydot128(ctx.poly, X[0], ctx.htbl[0]);
		X[0] = _mm_xor_si128(X[0], tmps[1]);
		X[0] = polydot128(ctx.poly, X[0], ctx.htbl[0]);

		X[1] = _mm_xor_si128(X[1], tmps[0]);
		X[1] = polydot128(ctx.poly, X[1], ctx.htbl[16]);
		X[1] = _mm_xor_si128(X[1], tmps[1]);
		X[1] = polydot128(ctx.poly, X[1], ctx.htbl[16]);

		addx2_1wise(inc, ctr, ctr);
		scatter_store_n2x1((__m128i *)C + dx8len * 16 + i * 2, tmps);
	}

	size_t Mrem = Mlen % 32;

	if (Mrem > 0)
	{
		uint8_t padded[32];
		memset(padded, 0, 32);
		memcpy(padded, M + Mlen - Mrem, Mrem);
		alignas(16) __m128i lastblk[2];
		lastblk[0] = _mm_loadu_si128((__m128i *)padded);
		lastblk[1] = _mm_loadu_si128((__m128i *)padded + 1);

		copyx2(ctr, tmps);
		xorx2_1wise(ctx.key, tmps, tmps);
		simpira_b2_128(ctx.simpira_ctx, tmps[0], tmps[1]);
		xorx2_1wise(ctx.key, tmps, tmps);
		xorx2_1wise(lastblk, tmps, tmps);

		memcpy((__m128i *)(C + dlen * 32), (uint8_t *)tmps, Mrem);

		((uint8_t *)&tmps)[Mrem] = 0x01;

		X[0] = _mm_xor_si128(X[0], tmps[0]);
		X[0] = polydot128(ctx.poly, X[0], ctx.htbl[0]);
		X[0] = _mm_xor_si128(X[0], tmps[1]);
		X[0] = polydot128(ctx.poly, X[0], ctx.htbl[0]);

		X[1] = _mm_xor_si128(X[1], tmps[0]);
		X[1] = polydot128(ctx.poly, X[1], ctx.htbl[16]);
		X[1] = _mm_xor_si128(X[1], tmps[1]);
		X[1] = polydot128(ctx.poly, X[1], ctx.htbl[16]);
	}

	hash[0] = X[0];
	hash[1] = X[1];
}
