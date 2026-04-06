#pragma once
#include "../common/common.h"

#include "core.h"

static inline __m128i polyreduce128(__m128i poly, __m128i x)
{
	alignas(16) __m128i x0 = _mm_clmulepi64_si128(x, poly, 0x10);
	alignas(16) __m128i y0 = _mm_shuffle_epi32(x, 78);
	alignas(16) __m128i y1 = _mm_xor_si128(y0, x0);
	alignas(16) __m128i x1 = _mm_clmulepi64_si128(y1, poly, 0x10);
	alignas(16) __m128i y2 = _mm_shuffle_epi32(y1, 78);
	return _mm_xor_si128(y2, x1);
}

static inline __m128i polydot128(__m128i poly, __m128i a, __m128i b)
{
	alignas(16) __m128i pp00 = _mm_clmulepi64_si128(a, b, 0x00);
	alignas(16) __m128i pp11 = _mm_clmulepi64_si128(a, b, 0x11);
	alignas(16) __m128i pp10 = _mm_clmulepi64_si128(a, b, 0x01);
	alignas(16) __m128i pp01 = _mm_clmulepi64_si128(a, b, 0x10);

	alignas(16) __m128i ppmid = _mm_xor_si128(pp10, pp01);
	alignas(16) __m128i ppmid_ls = _mm_bslli_si128(ppmid, 8);
	alignas(16) __m128i ppmid_rs = _mm_bsrli_si128(ppmid, 8);
	alignas(16) __m128i ppupper = _mm_xor_si128(ppmid_rs, pp11);
	alignas(16) __m128i pplower = _mm_xor_si128(ppmid_ls, pp00);
	alignas(16) __m128i ppl_reduced = polyreduce128(poly, pplower);
	return _mm_xor_si128(ppupper, ppl_reduced);
}

// required blen to be divisable by blocklength
static inline void polyval128n2(hctr2_context ctx, __m128i *state, uint8_t *blks, size_t blen, __m128i *hash)
{
	alignas(16) __m128i X[2] = {state[0], state[1]};
	alignas(16) __m128i data[2];
	alignas(16) __m128i tmps[2];

	size_t dlen = blen / 16;

	for (size_t i = 0; i < dlen; i++)
	{
		data[0] = _mm_loadu_si128((__m128i*)blks + i);
		X[0] = _mm_xor_si128(X[0], data[0]);
		X[1] = _mm_xor_si128(X[1], data[0]);
		X[0] = polydot128(ctx.poly, X[0], ctx.htbl[1]);
		X[1] = polydot128(ctx.poly, X[1], ctx.htbl[17]);
	}

	hash[0] = X[0];
	hash[1] = X[1];
}

static inline void polyval128n2x4(hctr2_context ctx, __m128i *state, uint8_t *blks, size_t blen, __m128i *hash)
{

	alignas(16) __m128i tmps[12];

	alignas(16) __m128i X[2] = {state[0], state[1]};
	alignas(16) __m128i Y[2] = {_mm_setzero_si128(), _mm_setzero_si128()};
	alignas(16) __m128i Z[2] = {_mm_setzero_si128(), _mm_setzero_si128()};
	alignas(16) __m128i data[4];

	size_t dlen = blen / 16;
	size_t dx4len = dlen / 4;
	size_t dx4rem = dlen % 4;

	for (size_t i = 0; i < dx4len; i++)
	{
		loadx4((__m128i*)blks + i * 4, data);

		Y[0] = polyreduce128(ctx.poly, Y[0]);
		Y[1] = polyreduce128(ctx.poly, Y[1]);
		Z[0] = _mm_xor_si128(X[0], Y[0]);
		Z[1] = _mm_xor_si128(X[1], Y[1]);
		Z[0] = _mm_xor_si128(Z[0], data[0]);
		Z[1] = _mm_xor_si128(Z[1], data[0]);

		schoolbook_initialadd128(data[3], ctx.htbl[1], tmps);
		schoolbook_initialadd128(data[3], ctx.htbl[17], tmps+4);

		schoolbook_add128(data[2], ctx.htbl[2], tmps);
		schoolbook_add128(data[2], ctx.htbl[18], tmps+4);

		schoolbook_add128(data[1], ctx.htbl[3], tmps);
		schoolbook_add128(data[1], ctx.htbl[19], tmps+4);

		schoolbook_add128(Z[0], ctx.htbl[4], tmps);
		schoolbook_add128(Z[1], ctx.htbl[20], tmps+4);

		tmps[3] = _mm_bsrli_si128(tmps[2], 8);
		tmps[2] = _mm_bslli_si128(tmps[2], 8);

		tmps[4 + 3] = _mm_bsrli_si128(tmps[4 + 2], 8);
		tmps[4 + 2] = _mm_bslli_si128(tmps[4 + 2], 8);

		X[0] = _mm_xor_si128(tmps[3], tmps[1]);
		Y[0] = _mm_xor_si128(tmps[0], tmps[2]);

		X[1] = _mm_xor_si128(tmps[4 + 3], tmps[4 + 1]);
		Y[1] = _mm_xor_si128(tmps[4 + 0], tmps[4 + 2]);
	}
	Y[0] = polyreduce128(ctx.poly, Y[0]);
	Z[0] = _mm_xor_si128(X[0], Y[0]);
	X[0] = Z[0];

	Y[1] = polyreduce128(ctx.poly, Y[1]);
	Z[1] = _mm_xor_si128(X[1], Y[1]);
	X[1] = Z[1];
	for (size_t i = 0; i < dx4rem; i++)
	{
		data[0] = _mm_loadu_si128((__m128i*)blks + dx4len * 4 + i);
		X[0] = _mm_xor_si128(X[0], data[0]);
		X[1] = _mm_xor_si128(X[1], data[0]);
		X[0] = polydot128(ctx.poly, X[0], ctx.htbl[1]);
		X[1] = polydot128(ctx.poly, X[1], ctx.htbl[17]);
	}
	hash[0] = X[0];
	hash[1] = X[1];
}

static inline void polyval128n2x8(hctr2_context ctx, __m128i* state, uint8_t *P, size_t Plen, __m128i *hash)
{
	alignas(16) __m128i X[2] = {state[0], state[1]};
	size_t remainder = Plen % 16;

	alignas(16) __m128i tmps[16];

	alignas(16) __m128i Y[2] = {_mm_setzero_si128(), _mm_setzero_si128()};
	alignas(16) __m128i Z[2] = {_mm_setzero_si128(), _mm_setzero_si128()};
	alignas(16) __m128i data[4];
	alignas(16) __m128i htbl8[16];

	for (size_t i = 0; i < 8; i++)
	{
			htbl8[i] = ctx.htbl[i + 1];
			htbl8[i + 8] = ctx.htbl[i + 17];
	}

	size_t loopnum = (Plen - remainder) / 16;
	for (size_t i = 0; i < (loopnum / 8); i++)
	{
		loadx8((__m128i*)P + i*8, data);

		Y[0] = polyreduce128(ctx.poly, Y[0]);
		Y[1] = polyreduce128(ctx.poly, Y[1]);
		Z[0] = _mm_xor_si128(X[0], Y[0]);
		Z[1] = _mm_xor_si128(X[1], Y[1]);

		Z[0] = _mm_xor_si128(Z[0], data[0]);
		Z[1] = _mm_xor_si128(Z[1], data[0]);

		mulinit_n2(data, htbl8, tmps, 8);

		muladd_n2(data, htbl8, tmps, 8, 1);
		muladd_n2(data, htbl8, tmps, 8, 2);
		muladd_n2(data, htbl8, tmps, 8, 3);
		muladd_n2(data, htbl8, tmps, 8, 4);
		muladd_n2(data, htbl8, tmps, 8, 5);
		muladd_n2(data, htbl8, tmps, 8, 6);

		muladdlast_n2(Z, htbl8, tmps, 8);

		tmps[3] = _mm_bsrli_si128(tmps[2], 8);
		tmps[2] = _mm_bslli_si128(tmps[2], 8);

		tmps[3 + 4] = _mm_bsrli_si128(tmps[2 + 4], 8);
		tmps[2 + 4] = _mm_bslli_si128(tmps[2 + 4], 8);

		X[0] = _mm_xor_si128(tmps[3], tmps[1]);
		Y[0] = _mm_xor_si128(tmps[0], tmps[2]);

		X[1] = _mm_xor_si128(tmps[3 + 4], tmps[1 + 4]);
		Y[1] = _mm_xor_si128(tmps[0 + 4], tmps[2 + 4]);
	}
	Y[0] = polyreduce128(ctx.poly, Y[0]);
	Z[0] = _mm_xor_si128(X[0], Y[0]);
	X[0] = Z[0];

	Y[1] = polyreduce128(ctx.poly, Y[1]);
	Z[1] = _mm_xor_si128(X[1], Y[1]);
	X[1] = Z[1];

	for (size_t i = 0; i < (loopnum % 8); i++)
	{
		data[0] = _mm_loadu_si128((__m128i *)P + loopnum - (loopnum % 8) + i);

		X[0] = _mm_xor_si128(X[0], data[0]);
		X[0] = polydot128(ctx.poly, X[0], ctx.htbl[1]);

		X[1] = _mm_xor_si128(X[1], data[0]);
		X[1] = polydot128(ctx.poly, X[1], ctx.htbl[17]);
	}

	hash[0] = X[0];
	hash[1] = X[1];
}

static inline void tweakhash128n2x4(hctr2_context ctx, bool mln, uint8_t *T, size_t Tlen, __m128i *state)
{
	size_t len = 2 * 8 * Tlen + 2 + mln;
	alignas(16) __m128i X[2] = {_mm_setzero_si128(), _mm_setzero_si128()};
	alignas(16) uint8_t first[32] = {};
        memcpy(first, &len, sizeof(len));
	polyval128n2(ctx, X, first, 32, X);

	size_t Trem = Tlen % 32;

	if (Tlen >= 32)
	{
		polyval128n2x4(ctx, X, T, Tlen - Trem, X);
	}
	if (Trem > 0)
	{
		uint8_t padded[32] = {};
                memcpy(padded, T + Tlen - Trem, Trem);
		polyval128n2(ctx, X, padded, 32, X);
	}

	state[0] = X[0];
	state[1] = X[1];
}

static inline void tweakhash128n2x8(hctr2_context ctx, bool mln, uint8_t *T, size_t Tlen, __m128i *state)
{
	size_t len = 2 * 8 * Tlen + 2 + mln;
	alignas(16) __m128i X[2] = {_mm_setzero_si128(), _mm_setzero_si128()};
	alignas(16) uint8_t first[32] = {};
        memcpy(first, &len, sizeof(len));
	polyval128n2(ctx, X, first, 32, X);

	size_t Trem = Tlen % 32;

	if (Tlen >= 32)
	{
		polyval128n2x8(ctx, X, T, Tlen - Trem, X);
	}
	if (Trem > 0)
	{
		uint8_t padded[32] = {};
                memcpy(padded, T + Tlen - Trem, Trem);
		polyval128n2(ctx, X, padded, 32, X);
	}

	state[0] = X[0];
	state[1] = X[1];
}

static inline void hash128x4(hctr2_context ctx, __m128i* state, uint8_t *M, size_t Mlen, __m128i* hash)
{
	alignas(16) __m128i X[2] = {state[0], state[1]};
	size_t Mrem = Mlen % 32;
	if (Mlen >= 32)
	{
		polyval128n2x4(ctx, state, M, Mlen - Mrem, X);
	}

	if (Mrem > 0)
	{
		uint8_t padded[32] = {};
                memcpy(padded, M + Mlen - Mrem, Mrem);
		padded[Mrem] = 0x01;
		polyval128n2(ctx, X, padded, 32, X);
	}

	hash[0] = X[0];
	hash[1] = X[1];
}

static inline void hash128x8(hctr2_context ctx, __m128i* state, uint8_t *M, size_t Mlen, __m128i* hash)
{
	alignas(16) __m128i X[2] = {state[0], state[1]};
	size_t Mrem = Mlen % 32;
	if (Mlen >= 32)
	{
		polyval128n2x8(ctx, state, M, Mlen - Mrem, X);
	}

	if (Mrem > 0)
	{
		uint8_t padded[32] = {};
                memcpy(padded, M + Mlen - Mrem, Mrem);
		padded[Mrem] = 0x01;
		polyval128n2(ctx, X, padded, 32, X);
	}

	hash[0] = X[0];
	hash[1] = X[1];
}
