#pragma once
#include "core.h"
#include <stdlib.h>

inline __m128i polyreduce128(__m128i poly, __m128i x)
{
	alignas(16) __m128i x0 = _mm_clmulepi64_si128(x, poly, 0x10);
	alignas(16) __m128i y0 = _mm_shuffle_epi32(x, 78);
	alignas(16) __m128i y1 = _mm_xor_si128(y0, x0);
	alignas(16) __m128i x1 = _mm_clmulepi64_si128(y1, poly, 0x10);
	alignas(16) __m128i y2 = _mm_shuffle_epi32(y1, 78);
	return _mm_xor_si128(y2, x1);
}

inline __m128i polydot128(__m128i poly, __m128i a, __m128i b)
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

static inline __m128i double128(__m128i poly, __m128i x)
{
	alignas(16) __m128i y;
	alignas(16) __m128i c = _mm_srli_epi64(x, 63);
	alignas(16) __m128i m0 = _mm_setr_epi32(1, 0, 0, 0);
	alignas(16) __m128i m1 = _mm_setr_epi32(0, 0, 1, 0);
	alignas(16) __m128i c0 = _mm_and_si128(c, m0);
	c0 = _mm_bslli_si128(c0, 8);
	alignas(16) __m128i c1 = _mm_and_si128(c, m1);
	c1 = _mm_bsrli_si128(c1, 8);
	y = _mm_slli_epi64(x, 1);
	y = _mm_xor_si128(y, c0);
	alignas(16) __m128i z = poly;
	z = _mm_clmulepi64_si128(c1, z, 0);
	y = _mm_xor_si128(y, z);
	return y;
}

static inline __m128i phash(aespoly_context ctx, const uint8_t *M, size_t Mlen)
{
	size_t blen = Mlen / 16;
	size_t brem = Mlen % 16;

	alignas(16) __m128i data[4];
	alignas(16) __m128i tmps[4];
	alignas(16) __m128i sum = _mm_setzero_si128();

	for (size_t i = 0; i < blen; i++)
	{
		data[0] = _mm_loadu_si128((__m128i *)M + i);
		tmps[0] = _mm_xor_si128(data[0], L[__tzcnt_u64(i + 1)]);
		sum = _mm_xor_si128(sum, tmps[0]);
	}

	if (brem)
	{
		uint8_t padded[16];
		memset(padded, 0, 16);
		memcpy(padded, M + 16 * blen, brem);
		padded[brem] = 0x80;
		alignas(16) __m128i blk = _mm_loadu_si128((__m128i *)padded);
		sum = _mm_xor_si128(sum, blk);
	}

	return sum;
}

#define tweaker_round(dataL, dataR, tmpsL, tmpsR, c, htbl, n, i) \
	{                                                          \
		simpira_roundx4(c[i * 4 + 1], tmpsR);                  \
		muladd_n2(dataL, htbl, tmpsL, n, 2 * i);               \
		simpira_roundx4_rev(c[i * 2 + 2], tmpsR);              \
		simpira_roundx4(c[i * 4 + 3], tmpsR);                  \
		muladd_n2(dataL, htbl, tmpsL, n, 2 * i + 1);           \
		simpira_roundx4_rev(c[i * 4 + 4], tmpsR);              \
	}

static inline void tweaker(aespoly_context ctx, bool mln, const uint8_t *T, size_t Tlen, __m128i *state, __m128i *hash)
{
	size_t m = (Tlen + 31) / 32;
	size_t rem = Tlen + 32 - m * 32;
	size_t dlen = MAX(m - m % 2 - 2, 0) / 2;
	size_t dx4rem = dlen % 4;
	size_t dx4len = dlen / 4;

	alignas(16) __m128i data[32];
	alignas(16) __m128i tmps[48];

	alignas(16) __m128i Li[2];

	__m128i *dataL = data;
	__m128i *dataR = data + 16;

	__m128i *tmpsL = tmps;
	__m128i *tmpsR = tmps + 16;
	__m128i *mask = tmps + 32;

	setzero_x16(mask);

	alignas(16) __m128i X[2];
	X[0] = _mm_setr_epi64(_m_from_int64(2 * Tlen + 2 + mln), _m_from_int64(0));
	X[0] = polydot128(ctx.poly, X[0], ctx.htbl[0]);
	X[1] = X[0];
	alignas(16) __m128i Y[2] = {_mm_setzero_si128(), _mm_setzero_si128()};
	alignas(16) __m128i Z[2] = {_mm_setzero_si128(), _mm_setzero_si128()};

	alignas(16) __m128i sum[2] = {_mm_setzero_si128(), _mm_setzero_si128()};

	if (Tlen < 32)
	{
		state[0] = X[0];
		state[1] = X[1];
		hash[0] = _mm_setzero_si128();
		hash[1] = _mm_setzero_si128();
		return;
	}

	copyx2(L, Li);

	for (size_t i = 0; i < dx4len; i++)
	{
		gather_load_n2x4((__m128i *)T + 16 * i + 1, dataL);
		gather_load_n2x4((__m128i *)T + 16 * i, dataR);

		seq_graycode_n2x4(mask, L, L + 2, Li, i);

		Y[0] = polyreduce128(ctx.poly, Y[0]);
		Y[1] = polyreduce128(ctx.poly, Y[1]);
		Z[0] = _mm_xor_si128(X[0], Y[0]);
		Z[1] = _mm_xor_si128(X[1], Y[1]);
		Z[0] = _mm_xor_si128(Z[0], dataL[0]);
		Z[1] = _mm_xor_si128(Z[1], dataL[0]);

		xorx8_1wise(dataR, mask, tmpsR);
		addkey256x4(ctx.simpira_ctx.keys, tmpsR, tmpsR);

		simpira_roundx4(ctx.simpira_ctx.c[1], tmpsR);
		mulinit_n2(dataL, ctx.htbl, tmpsL, 8);
		simpira_roundx4_rev(ctx.simpira_ctx.c[2], tmpsR);
		simpira_roundx4(ctx.simpira_ctx.c[3], tmpsR);
		muladd_n2(dataL, ctx.htbl, tmpsL, 8, 1);
		simpira_roundx4_rev(ctx.simpira_ctx.c[4], tmpsR);

		tweaker_round(dataL, dataR, tmpsL, tmpsR, ctx.simpira_ctx.c, ctx.htbl, 8, 1);
		tweaker_round(dataL, dataR, tmpsL, tmpsR, ctx.simpira_ctx.c, ctx.htbl, 8, 2);

		simpira_roundx4(ctx.simpira_ctx.c[13], tmpsR);
		muladd_n2(dataL, ctx.htbl, tmpsL, 8, 6);
		simpira_roundx4_rev(ctx.simpira_ctx.c[14], tmpsR);
		simpira_roundx4(ctx.simpira_ctx.c[15], tmpsR);
		muladdlast_n2(Z, ctx.htbl, tmpsL, 8);
		addkey256x4(ctx.simpira_ctx.keys, tmpsR, tmpsR);

		sum_n2x4(tmpsR, sum);

		tmpsL[3] = _mm_bsrli_si128(tmpsL[2], 8);
		tmpsL[2] = _mm_bslli_si128(tmpsL[2], 8);

		tmpsL[4 + 3] = _mm_bsrli_si128(tmpsL[4 + 2], 8);
		tmpsL[4 + 2] = _mm_bslli_si128(tmpsL[4 + 2], 8);

		X[0] = _mm_xor_si128(tmpsL[3], tmpsL[1]);
		Y[0] = _mm_xor_si128(tmpsL[0], tmpsL[2]);

		X[1] = _mm_xor_si128(tmpsL[4 + 3], tmpsL[4 + 1]);
		Y[1] = _mm_xor_si128(tmpsL[4 + 0], tmpsL[4 + 2]);
	}
	Y[0] = polyreduce128(ctx.poly, Y[0]);
	Z[0] = _mm_xor_si128(X[0], Y[0]);
	X[0] = Z[0];

	Y[1] = polyreduce128(ctx.poly, Y[1]);
	Z[1] = _mm_xor_si128(X[1], Y[1]);
	X[1] = Z[1];
	for (size_t i = 0; i < dx4rem; i++)
	{
		gather_load_n2x1((__m128i *)T + 16 * dx4len + 4 * i + 1, dataL);
		gather_load_n2x1((__m128i *)T + 16 * dx4len + 4 * i, dataR);

		seq_graycode_n2x1(mask, L, dx4len * 4 + i);

		xorx2_1wise(dataR, mask, tmpsR);
		addkey256(ctx.simpira_ctx.keys, tmpsR, tmpsR);
		simpira_b2x1_128(ctx.simpira_ctx, tmpsR);
		addkey256(ctx.simpira_ctx.keys, tmpsR, tmpsR);

		sum[0] = _mm_xor_si128(sum[0], tmpsR[0]);
		sum[1] = _mm_xor_si128(sum[1], tmpsR[1]);

		X[0] = _mm_xor_si128(X[0], dataL[0]);
		X[0] = polydot128(ctx.poly, X[0], ctx.htbl[0]);
		X[0] = _mm_xor_si128(X[0], dataL[1]);
		X[0] = polydot128(ctx.poly, X[0], ctx.htbl[0]);

		X[1] = _mm_xor_si128(X[1], dataL[0]);
		X[1] = polydot128(ctx.poly, X[1], ctx.htbl[0]);
		X[1] = _mm_xor_si128(X[1], dataL[1]);
		X[1] = polydot128(ctx.poly, X[1], ctx.htbl[0]);
	}

	if (Tlen == 32)
	{
		state[0] = X[0];
		state[1] = X[1];
		hash[0] = _mm_loadu_si128((__m128i *)T);
		hash[1] = _mm_loadu_si128((__m128i *)T + 1);
	}
	else
	{
		if (m % 2 == 0)
		{
			data[0] = _mm_loadu_si128((__m128i *)T + dlen * 4);
			data[1] = _mm_loadu_si128((__m128i *)T + dlen * 4 + 1);

			uint8_t padded[32];
			memset(padded, 0, 32);
			memcpy(padded, T + Tlen - rem, rem);
			// padded[rem] = 0x80;
			alignas(16) __m128i paddedblk[2];
			paddedblk[0] = _mm_loadu_si128((__m128i *)padded);
			paddedblk[1] = _mm_loadu_si128((__m128i *)padded + 1);

			seq_graycode_n2x1(mask, L, dlen);

			xorx2_1wise(data, mask, tmpsR);
			addkey256(ctx.simpira_ctx.keys, tmpsR, tmpsR);
			simpira_b2x1_128(ctx.simpira_ctx, tmpsR);
			addkey256(ctx.simpira_ctx.keys, tmpsR, tmpsR);

			hash[0] = _mm_xor_si128(sum[0], tmpsR[0]);
			hash[1] = _mm_xor_si128(sum[1], tmpsR[1]);

			X[0] = _mm_xor_si128(X[0], paddedblk[0]);
			X[0] = polydot128(ctx.poly, X[0], ctx.htbl[0]);
			X[0] = _mm_xor_si128(X[0], paddedblk[1]);
			state[0] = polydot128(ctx.poly, X[0], ctx.htbl[0]);

			X[1] = _mm_xor_si128(X[1], paddedblk[0]);
			X[1] = polydot128(ctx.poly, X[1], ctx.htbl[0]);
			X[1] = _mm_xor_si128(X[1], paddedblk[1]);
			state[1] = polydot128(ctx.poly, X[1], ctx.htbl[0]);
		}
		else
		{
			data[0] = _mm_loadu_si128((__m128i *)T + dlen * 2);
			data[1] = _mm_loadu_si128((__m128i *)T + dlen * 2 + 1);
			data[2] = _mm_loadu_si128((__m128i *)(T + Tlen - 32));
			data[3] = _mm_loadu_si128((__m128i *)(T + Tlen - 32) + 1);

			uint8_t padded[32];
			memset(padded, 0, 32);
			memcpy(padded, T + Tlen - 32 - rem, rem);
			// padded[rem] = 0x80;
			alignas(16) __m128i paddedblk[2];
			paddedblk[0] = _mm_loadu_si128((__m128i *)padded);
			paddedblk[1] = _mm_loadu_si128((__m128i *)padded + 1);

			seq_graycode_n2x1(mask, L, dlen);

			xorx2_1wise(data, mask, tmpsR);
			addkey256(ctx.key, tmpsR, tmpsR);
			simpira_b2x1_128(ctx.simpira_ctx, tmps);
			addkey256(ctx.key, tmpsR, tmpsR);
			sum[0] = _mm_xor_si128(sum[0], tmps[0]);
			sum[1] = _mm_xor_si128(sum[1], tmps[1]);

			seq_graycode_n2x1(mask, L, dlen + 1);

			xorx2_1wise(data + 2, mask, tmpsR);
			addkey256(ctx.key, tmpsR, tmpsR);
			simpira_b2x1_128(ctx.simpira_ctx, tmps);
			addkey256(ctx.key, tmpsR, tmpsR);
			sum[0] = _mm_xor_si128(sum[0], tmps[0]);
			sum[1] = _mm_xor_si128(sum[1], tmps[1]);

			X[0] = _mm_xor_si128(X[0], paddedblk[0]);
			X[0] = polydot128(ctx.poly, X[0], ctx.htbl[0]);
			X[0] = _mm_xor_si128(X[0], paddedblk[1]);
			state[0] = polydot128(ctx.poly, X[0], ctx.htbl[0]);

			X[1] = _mm_xor_si128(X[1], paddedblk[0]);
			X[1] = polydot128(ctx.poly, X[1], ctx.htbl[0]);
			X[1] = _mm_xor_si128(X[1], paddedblk[1]);
			state[1] = polydot128(ctx.poly, X[1], ctx.htbl[0]);

			hash[0] = sum[0];
			hash[1] = sum[1];
		}
	}
}

#define upper_round(dataL, dataR, tmpsL, tmpsR, c, htbl, n, i) \
	{                                                          \
		simpira_roundx4(c[i * 4 + 1], tmpsR);                  \
		muladd_n2(dataL, htbl, tmpsL, n, 2 * i);               \
		simpira_roundx4_rev(c[i * 2 + 2], tmpsR);              \
		simpira_roundx4(c[i * 4 + 3], tmpsR);                  \
		muladd_n2(dataL, htbl, tmpsL, n, 2 * i + 1);           \
		simpira_roundx4_rev(c[i * 4 + 4], tmpsR);              \
	}

#define upper_roundx8(dataL, dataR, tmpsL, tmpsR, c, htbl, n, i) \
	{                                                          \
		simpira_roundx8(c[i * 4 + 1], tmpsR);                  \
		muladd_n2(dataL, htbl, tmpsL, n, 2 * i);               \
		simpira_roundx8_rev(c[i * 2 + 2], tmpsR);              \
		simpira_roundx8(c[i * 4 + 3], tmpsR);                  \
		muladd_n2(dataL, htbl, tmpsL, n, 2 * i + 1);           \
		simpira_roundx8_rev(c[i * 4 + 4], tmpsR);              \
	}

static inline void upper(aespoly_context ctx, __m128i *state, const uint8_t *M, size_t Mlen, uint8_t *C, __m128i *hash, __m128i *sum)
{
	size_t m = (Mlen + 31) / 32;
	size_t rem = Mlen + 32 - m * 32;
	size_t dlen = MAX(m - 1, 0) / 2;
	size_t dx4rem = dlen % 4;
	size_t dx4len = dlen / 4;

	alignas(16) __m128i data[32];
	alignas(16) __m128i tmps[48];

	alignas(16) __m128i omegai[2];

	__m128i *dataL = data;
	__m128i *dataR = data + 16;

	__m128i *tmpsL = tmps;
	__m128i *tmpsR = tmps + 16;
	__m128i *mask = tmps + 32;

	setzero_x16(mask);

	alignas(16) __m128i X[2] = {state[0], state[1]};
	alignas(16) __m128i Y[2] = {_mm_setzero_si128(), _mm_setzero_si128()};
	alignas(16) __m128i Z[2] = {_mm_setzero_si128(), _mm_setzero_si128()};

	sum[0] = _mm_setzero_si128();
	sum[1] = _mm_setzero_si128();

	if (Mlen < 32)
	{
		return;
	}

	copyx2(omega, omegai);

	for (size_t i = 0; i < dx4len; i++)
	{
		gather_load_n2x4((__m128i *)M + 16 * i, dataL);
		gather_load_n2x4((__m128i *)M + 16 * i + 1, dataR);

		seq_graycode_n2x4(mask, omega, omega + 2, omegai, i);

		Y[0] = polyreduce128(ctx.poly, Y[0]);
		Y[1] = polyreduce128(ctx.poly, Y[1]);
		Z[0] = _mm_xor_si128(X[0], Y[0]);
		Z[1] = _mm_xor_si128(X[1], Y[1]);

		Z[0] = _mm_xor_si128(Z[0], dataL[0]);
		Z[1] = _mm_xor_si128(Z[1], dataL[0]);

		xorx8_1wise(dataR, mask, tmpsR);
		addkey256x4(ctx.simpira_ctx.keys, tmpsR, tmpsR);

		simpira_roundx4(ctx.simpira_ctx.c[1], tmpsR);
		mulinit_n2(dataL, ctx.htbl, tmpsL, 8);
		simpira_roundx4_rev(ctx.simpira_ctx.c[2], tmpsR);
		simpira_roundx4(ctx.simpira_ctx.c[3], tmpsR);
		muladd_n2(dataL, ctx.htbl, tmpsL, 8, 1);
		simpira_roundx4_rev(ctx.simpira_ctx.c[4], tmpsR);

		upper_round(dataL, dataR, tmpsL, tmpsR, ctx.simpira_ctx.c, ctx.htbl, 8, 1);
		upper_round(dataL, dataR, tmpsL, tmpsR, ctx.simpira_ctx.c, ctx.htbl, 8, 2);

		simpira_roundx4(ctx.simpira_ctx.c[13], tmpsR);
		muladd_n2(dataL, ctx.htbl, tmpsL, 8, 6);
		simpira_roundx4_rev(ctx.simpira_ctx.c[14], tmpsR);
		simpira_roundx4(ctx.simpira_ctx.c[15], tmpsR);
		muladdlast_n2(Z, ctx.htbl, tmpsL, 8);
		addkey256x4(ctx.simpira_ctx.keys, tmpsR, tmpsR);

		sum_n2x4(tmpsR, sum);

		tmpsL[3] = _mm_bsrli_si128(tmpsL[2], 8);
		tmpsL[2] = _mm_bslli_si128(tmpsL[2], 8);

		tmpsL[4 + 3] = _mm_bsrli_si128(tmpsL[4 + 2], 8);
		tmpsL[4 + 2] = _mm_bslli_si128(tmpsL[4 + 2], 8);

		X[0] = _mm_xor_si128(tmpsL[3], tmpsL[1]);
		Y[0] = _mm_xor_si128(tmpsL[0], tmpsL[2]);

		X[1] = _mm_xor_si128(tmpsL[4 + 3], tmpsL[4 + 1]);
		Y[1] = _mm_xor_si128(tmpsL[4 + 0], tmpsL[4 + 2]);

		scatter_store_n2x4((__m128i *)C + 16 * i + 1, tmpsR);
	}
	Y[0] = polyreduce128(ctx.poly, Y[0]);
	Z[0] = _mm_xor_si128(X[0], Y[0]);
	X[0] = Z[0];

	Y[1] = polyreduce128(ctx.poly, Y[1]);
	Z[1] = _mm_xor_si128(X[1], Y[1]);
	X[1] = Z[1];

	for (size_t i = 0; i < dx4rem; i++)
	{
		gather_load_n2x1((__m128i *)M + 16 * dx4len + 4 * i, dataL);
		gather_load_n2x1((__m128i *)M + 16 * dx4len + 4 * i + 1, dataR);

		seq_graycode_n2x1(mask, omega, dx4len * 4 + i);

		xorx2_1wise(dataR, mask, tmpsR);
		addkey256(ctx.simpira_ctx.keys, tmpsR, tmpsR);
		simpira_b2x1_128(ctx.simpira_ctx, tmpsR);
		addkey256(ctx.simpira_ctx.keys, tmpsR, tmpsR);

		sum[0] = _mm_xor_si128(sum[0], tmpsR[0]);
		sum[1] = _mm_xor_si128(sum[1], tmpsR[1]);

		X[0] = _mm_xor_si128(X[0], dataL[0]);
		X[0] = polydot128(ctx.poly, X[0], ctx.htbl[0]);
		X[0] = _mm_xor_si128(X[0], dataL[1]);
		X[0] = polydot128(ctx.poly, X[0], ctx.htbl[0]);

		X[1] = _mm_xor_si128(X[1], dataL[0]);
		X[1] = polydot128(ctx.poly, X[1], ctx.htbl[0]);
		X[1] = _mm_xor_si128(X[1], dataL[1]);
		X[1] = polydot128(ctx.poly, X[1], ctx.htbl[0]);

		scatter_store_n2x1((__m128i *)C + 16 * dx4len + 4 * i + 1, tmpsR);
	}

	if (m % 2 == 0)
	{
		data[0] = _mm_loadu_si128((__m128i *)(M + Mlen - 32));
		data[1] = _mm_loadu_si128((__m128i *)(M + Mlen - 32) + 1);

		uint8_t padded[33];
		memset(padded, 0, 32);
		memcpy(padded, M + Mlen - 32 - rem, rem);
		padded[rem] = 0x80;
		alignas(16) __m128i paddedblk[2];
		paddedblk[0] = _mm_loadu_si128((__m128i *)padded);
		paddedblk[1] = _mm_loadu_si128((__m128i *)padded + 1);

		seq_graycode_n2x1(mask, omega, dlen);

		xorx2_1wise(data, mask, tmpsR);
		addkey256(ctx.simpira_ctx.keys, tmpsR, tmpsR);
		simpira_b2x1_128(ctx.simpira_ctx, tmpsR);
		addkey256(ctx.simpira_ctx.keys, tmpsR, tmpsR);

		sum[0] = _mm_xor_si128(sum[0], tmpsR[0]);
		sum[1] = _mm_xor_si128(sum[1], tmpsR[1]);

		scatter_store_n2x1((__m128i *)(C + Mlen - 32), tmpsR);

		X[0] = _mm_xor_si128(X[0], paddedblk[0]);
		X[0] = polydot128(ctx.poly, X[0], ctx.htbl[0]);
		X[0] = _mm_xor_si128(X[0], paddedblk[1]);
		hash[0] = polydot128(ctx.poly, X[0], ctx.htbl[0]);

		X[1] = _mm_xor_si128(X[1], paddedblk[0]);
		X[1] = polydot128(ctx.poly, X[1], ctx.htbl[0]);
		X[1] = _mm_xor_si128(X[1], paddedblk[1]);
		hash[1] = polydot128(ctx.poly, X[1], ctx.htbl[0]);
	}
	else
	{
		uint8_t padded[33];
		memset(padded, 0, 32);
		memcpy(padded, M + Mlen - rem, rem);
		padded[rem] = 0x80;
		alignas(16) __m128i paddedblk[2];
		paddedblk[0] = _mm_loadu_si128((__m128i *)padded);
		paddedblk[1] = _mm_loadu_si128((__m128i *)padded + 1);

		X[0] = _mm_xor_si128(X[0], paddedblk[0]);
		X[0] = polydot128(ctx.poly, X[0], ctx.htbl[0]);
		X[0] = _mm_xor_si128(X[0], paddedblk[1]);
		hash[0] = polydot128(ctx.poly, X[0], ctx.htbl[0]);

		X[1] = _mm_xor_si128(X[1], paddedblk[0]);
		X[1] = polydot128(ctx.poly, X[1], ctx.htbl[0]);
		X[1] = _mm_xor_si128(X[1], paddedblk[1]);
		hash[1] = polydot128(ctx.poly, X[1], ctx.htbl[0]);
	}
}

static inline void upperx8(aespoly_context ctx, __m128i *state, const uint8_t *M, size_t Mlen, uint8_t *C, __m128i *hash, __m128i *sum)
{
	size_t m = (Mlen + 31) / 32;
	size_t rem = Mlen + 32 - m * 32;
	size_t dlen = MAX(m - 1, 0) / 2;
	size_t dx8rem = dlen % 8;
	size_t dx8len = dlen / 8;

	alignas(16) __m128i data[32];
	alignas(16) __m128i tmps[48];

	alignas(16) __m128i omegai[8];

	__m128i *dataL = data;
	__m128i *dataR = data + 16;

	__m128i *tmpsL = tmps;
	__m128i *tmpsR = tmps + 16;
	__m128i *mask = tmps + 32;

	setzero_x16(mask);

	alignas(16) __m128i X[2] = {state[0], state[1]};
	alignas(16) __m128i Y[2] = {_mm_setzero_si128(), _mm_setzero_si128()};
	alignas(16) __m128i Z[2] = {_mm_setzero_si128(), _mm_setzero_si128()};

	sum[0] = _mm_setzero_si128();
	sum[1] = _mm_setzero_si128();

	if (Mlen < 32)
	{
		return;
	}

	copyx2(omega, omegai);

	for (size_t i = 0; i < dx8len; i++)
	{
		gather_load_n2x8((__m128i *)M + 32 * i, dataL);
		gather_load_n2x8((__m128i *)M + 32 * i + 1, dataR);

		seq_graycode_n2x8(mask, omegai, i);

		Y[0] = polyreduce128(ctx.poly, Y[0]);
		Y[1] = polyreduce128(ctx.poly, Y[1]);
		Z[0] = _mm_xor_si128(X[0], Y[0]);
		Z[1] = _mm_xor_si128(X[1], Y[1]);

		Z[0] = _mm_xor_si128(Z[0], dataL[0]);
		Z[1] = _mm_xor_si128(Z[1], dataL[0]);

		xorx16_1wise(dataR, mask, tmpsR);
		addkey256x8(ctx.simpira_ctx.keys, tmpsR, tmpsR);

		simpira_roundx8(ctx.simpira_ctx.c[1], tmpsR);
		mulinit_n2(dataL, ctx.htbl, tmpsL, 16);
		simpira_roundx8_rev(ctx.simpira_ctx.c[2], tmpsR);
		simpira_roundx8(ctx.simpira_ctx.c[3], tmpsR);
		muladd_n2(dataL, ctx.htbl, tmpsL, 16, 1);
		simpira_roundx8_rev(ctx.simpira_ctx.c[4], tmpsR);

		upper_roundx8(dataL, dataR, tmpsL, tmpsR, ctx.simpira_ctx.c, ctx.htbl, 16, 1);
		upper_roundx8(dataL, dataR, tmpsL, tmpsR, ctx.simpira_ctx.c, ctx.htbl, 16, 2);

		simpira_roundx8(ctx.simpira_ctx.c[13], tmpsR);
		muladd_n2(dataL, ctx.htbl, tmpsL, 16, 6);
		simpira_roundx8_rev(ctx.simpira_ctx.c[14], tmpsR);
		simpira_roundx8(ctx.simpira_ctx.c[15], tmpsR);
		
		muladd_n2(dataL, ctx.htbl, tmpsL, 16, 7);
		muladd_n2(dataL, ctx.htbl, tmpsL, 16, 8);
		muladd_n2(dataL, ctx.htbl, tmpsL, 16, 9);
		muladd_n2(dataL, ctx.htbl, tmpsL, 16, 10);
		muladd_n2(dataL, ctx.htbl, tmpsL, 16, 11);
		muladd_n2(dataL, ctx.htbl, tmpsL, 16, 12);
		muladd_n2(dataL, ctx.htbl, tmpsL, 16, 13);
		muladd_n2(dataL, ctx.htbl, tmpsL, 16, 14);
		muladdlast_n2(Z, ctx.htbl, tmpsL, 8);
		addkey256x8(ctx.simpira_ctx.keys, tmpsR, tmpsR);

		sum_n2x8(tmpsR, sum);

		tmpsL[3] = _mm_bsrli_si128(tmpsL[2], 8);
		tmpsL[2] = _mm_bslli_si128(tmpsL[2], 8);

		tmpsL[4 + 3] = _mm_bsrli_si128(tmpsL[4 + 2], 8);
		tmpsL[4 + 2] = _mm_bslli_si128(tmpsL[4 + 2], 8);

		X[0] = _mm_xor_si128(tmpsL[3], tmpsL[1]);
		Y[0] = _mm_xor_si128(tmpsL[0], tmpsL[2]);

		X[1] = _mm_xor_si128(tmpsL[4 + 3], tmpsL[4 + 1]);
		Y[1] = _mm_xor_si128(tmpsL[4 + 0], tmpsL[4 + 2]);

		scatter_store_n2x8((__m128i *)C + 32 * i + 1, tmpsR);
	}
	Y[0] = polyreduce128(ctx.poly, Y[0]);
	Z[0] = _mm_xor_si128(X[0], Y[0]);
	X[0] = Z[0];

	Y[1] = polyreduce128(ctx.poly, Y[1]);
	Z[1] = _mm_xor_si128(X[1], Y[1]);
	X[1] = Z[1];

	for (size_t i = 0; i < dx8rem; i++)
	{
		gather_load_n2x1((__m128i *)M + 32 * dx8len + 4 * i, dataL);
		gather_load_n2x1((__m128i *)M + 32 * dx8len + 4 * i + 1, dataR);

		seq_graycode_n2x1(mask, omegai, dx8len * 8 + i);

		xorx2_1wise(dataR, mask, tmpsR);
		addkey256(ctx.simpira_ctx.keys, tmpsR, tmpsR);
		simpira_b2x1_128(ctx.simpira_ctx, tmpsR);
		addkey256(ctx.simpira_ctx.keys, tmpsR, tmpsR);

		sum[0] = _mm_xor_si128(sum[0], tmpsR[0]);
		sum[1] = _mm_xor_si128(sum[1], tmpsR[1]);

		X[0] = _mm_xor_si128(X[0], dataL[0]);
		X[0] = polydot128(ctx.poly, X[0], ctx.htbl[0]);
		X[0] = _mm_xor_si128(X[0], dataL[1]);
		X[0] = polydot128(ctx.poly, X[0], ctx.htbl[0]);

		X[1] = _mm_xor_si128(X[1], dataL[0]);
		X[1] = polydot128(ctx.poly, X[1], ctx.htbl[0]);
		X[1] = _mm_xor_si128(X[1], dataL[1]);
		X[1] = polydot128(ctx.poly, X[1], ctx.htbl[0]);

		scatter_store_n2x1((__m128i *)C + 32 * dx8len + 4 * i + 1, tmpsR);
	}

	if (m % 2 == 0)
	{
		data[0] = _mm_loadu_si128((__m128i *)(M + Mlen - 32));
		data[1] = _mm_loadu_si128((__m128i *)(M + Mlen - 32) + 1);

		uint8_t padded[33];
		memset(padded, 0, 32);
		memcpy(padded, M + Mlen - 32 - rem, rem);
		padded[rem] = 0x80;
		alignas(16) __m128i paddedblk[2];
		paddedblk[0] = _mm_loadu_si128((__m128i *)padded);
		paddedblk[1] = _mm_loadu_si128((__m128i *)padded + 1);

		seq_graycode_n2x1(mask, omegai, dlen);

		xorx2_1wise(data, mask, tmpsR);
		addkey256(ctx.simpira_ctx.keys, tmpsR, tmpsR);
		simpira_b2x1_128(ctx.simpira_ctx, tmpsR);
		addkey256(ctx.simpira_ctx.keys, tmpsR, tmpsR);

		sum[0] = _mm_xor_si128(sum[0], tmpsR[0]);
		sum[1] = _mm_xor_si128(sum[1], tmpsR[1]);

		scatter_store_n2x1((__m128i *)(C + Mlen - 32), tmpsR);

		X[0] = _mm_xor_si128(X[0], paddedblk[0]);
		X[0] = polydot128(ctx.poly, X[0], ctx.htbl[0]);
		X[0] = _mm_xor_si128(X[0], paddedblk[1]);
		hash[0] = polydot128(ctx.poly, X[0], ctx.htbl[0]);

		X[1] = _mm_xor_si128(X[1], paddedblk[0]);
		X[1] = polydot128(ctx.poly, X[1], ctx.htbl[0]);
		X[1] = _mm_xor_si128(X[1], paddedblk[1]);
		hash[1] = polydot128(ctx.poly, X[1], ctx.htbl[0]);
	}
	else
	{
		uint8_t padded[33];
		memset(padded, 0, 32);
		memcpy(padded, M + Mlen - rem, rem);
		padded[rem] = 0x80;
		alignas(16) __m128i paddedblk[2];
		paddedblk[0] = _mm_loadu_si128((__m128i *)padded);
		paddedblk[1] = _mm_loadu_si128((__m128i *)padded + 1);

		X[0] = _mm_xor_si128(X[0], paddedblk[0]);
		X[0] = polydot128(ctx.poly, X[0], ctx.htbl[0]);
		X[0] = _mm_xor_si128(X[0], paddedblk[1]);
		hash[0] = polydot128(ctx.poly, X[0], ctx.htbl[0]);

		X[1] = _mm_xor_si128(X[1], paddedblk[0]);
		X[1] = polydot128(ctx.poly, X[1], ctx.htbl[0]);
		X[1] = _mm_xor_si128(X[1], paddedblk[1]);
		hash[1] = polydot128(ctx.poly, X[1], ctx.htbl[0]);
	}
}

#define lower_round(dataL, dataR, tmpsL, tmpsR, c, htbl, n, i) \
	{                                                          \
		simpira_roundx4(c[i * 4 + 1], tmpsR);                  \
		muladd_n2(dataL, htbl, tmpsL, n, 2 * i);               \
		simpira_roundx4_rev(c[i * 4 + 2], tmpsR);              \
		simpira_roundx4(c[i * 4 + 3], tmpsR);                  \
		muladd_n2(dataL, htbl, tmpsL, n, 2 * i + 1);           \
		simpira_roundx4_rev(c[i * 4 + 4], tmpsR);              \
	}

#define lower_roundx8(dataL, dataR, tmpsL, tmpsR, c, htbl, n, i) \
	{                                                          \
		simpira_roundx8(c[i * 4 + 1], tmpsR);                  \
		muladd_n2(dataL, htbl, tmpsL, n, 2 * i);               \
		simpira_roundx8_rev(c[i * 4 + 2], tmpsR);              \
		simpira_roundx8(c[i * 4 + 3], tmpsR);                  \
		muladd_n2(dataL, htbl, tmpsL, n, 2 * i + 1);           \
		simpira_roundx8_rev(c[i * 4 + 4], tmpsR);              \
	}

static inline void middlelower(aespoly_context ctx, __m128i *state, __m128i *S1, __m128i *S2, const uint8_t *M, size_t Mlen, uint8_t *C, __m128i *hash)
{
	size_t m = (Mlen + 31) / 32;
	size_t rem = Mlen + 32 - m * 32;
	size_t dlen = MAX(m - 1, 0) / 2;
	size_t dx4rem = dlen % 4;
	size_t dx4len = dlen / 4;


	alignas(16) __m128i data[32];
	alignas(16) __m128i tmps[64];
	alignas(16) __m128i omegai[2];

	__m128i *dataL = data;
	__m128i *dataR = data + 16;

	__m128i *tmpsL = tmps;
	__m128i *tmpsR = tmps + 16;
	__m128i *mask = tmps + 32;
	__m128i *tmpsH = tmps + 48;

	setzero_x16(mask);
	setzero_x16(tmpsL);

	alignas(16) __m128i X[2] = {state[0], state[1]};
	alignas(16) __m128i Y[2] = {_mm_setzero_si128(), _mm_setzero_si128()};
	alignas(16) __m128i Z[2] = {_mm_setzero_si128(), _mm_setzero_si128()};

	alignas(16) __m128i ctr[16];
	alignas(16) __m128i inc4[16];
	alignas(16) __m128i inc[16];

	for (size_t i = 0; i < 4; i++)
	{
		ctr[2 * i] = _mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S1[0]);
		ctr[2 * i + 1] = _mm_xor_si128(_mm_setr_epi32(i + 1, 0, 0, 0), S1[1]);
		inc4[2 * i] = _mm_srli_si128(_mm_setr_epi32(1 << 2, 0, 0, 0), 2);
		inc4[2 * i + 1] = _mm_setr_epi32(4, 0, 0, 0);
		inc[2 * i] = _mm_srli_si128(_mm_setr_epi32(1 << 2, 0, 0, 0), 2);
		inc[2 * i + 1] = _mm_setr_epi32(1, 0, 0, 0);
	}

	if (Mlen < 32)
	{
		return;
	}

	copyx2(omega, omegai);

	for (size_t i = 0; i < dx4len; i++)
	{
		gather_load_n2x4((__m128i *)M + 16 * i, dataL);
		gather_load_n2x4((__m128i *)M + 16 * i + 1, dataR);

		seq_graycode_n2x4(mask, omega, omega + 2, omegai, i);

		Y[0] = polyreduce128(ctx.poly, Y[0]);
		Y[1] = polyreduce128(ctx.poly, Y[1]);
		Z[0] = _mm_xor_si128(X[0], Y[0]);
		Z[1] = _mm_xor_si128(X[1], Y[1]);

		addkey256x4(S1, ctr, tmpsL);
		addkey256x4(ctx.simpira_ctx.keys, tmpsL, tmpsL);
		simpira_b2x4_128(ctx.simpira_ctx, tmpsL);
		addkey256x4(ctx.simpira_ctx.keys, tmpsL, tmpsL);
		xorx8_1wise(tmpsL, dataL, tmpsL);
		addkey256x4(S2, dataR, tmpsR);

		Z[0] = _mm_xor_si128(Z[0], tmpsL[0]);
		Z[1] = _mm_xor_si128(Z[1], tmpsL[0]);

		simpira_roundx4(ctx.simpira_ctx.c[1], tmpsR);
		mulinit_n2(tmpsL, ctx.htbl, tmpsH, 8);
		simpira_roundx4_rev(ctx.simpira_ctx.c[2], tmpsR);
		simpira_roundx4(ctx.simpira_ctx.c[3], tmpsR);
		muladd_n2(tmpsL, ctx.htbl, tmpsH, 8, 1);
		simpira_roundx4_rev(ctx.simpira_ctx.c[4], tmpsR);

		lower_round(tmpsL, tmpsR, tmpsH, tmpsR, ctx.simpira_ctx.c, ctx.htbl, 8, 1);
		lower_round(tmpsL, tmpsR, tmpsH, tmpsR, ctx.simpira_ctx.c, ctx.htbl, 8, 2);

		simpira_roundx4(ctx.simpira_ctx.c[13], tmpsR);
		muladd_n2(tmpsL, ctx.htbl, tmpsH, 8, 6);
		simpira_roundx4_rev(ctx.simpira_ctx.c[14], tmpsR);
		simpira_roundx4(ctx.simpira_ctx.c[15], tmpsR);
		muladd_n2(tmpsL, ctx.htbl, tmpsH, 8, 7);
		muladdlast_n2(Z, ctx.htbl, tmpsH, 8);
		addkey256x4(ctx.simpira_ctx.keys, tmpsR, tmpsR);

		xorx8_1wise(tmpsR, mask, tmpsR);

		tmpsH[3] = _mm_bsrli_si128(tmpsH[2], 8);
		tmpsH[2] = _mm_bslli_si128(tmpsH[2], 8);

		tmpsH[4 + 3] = _mm_bsrli_si128(tmpsH[4 + 2], 8);
		tmpsH[4 + 2] = _mm_bslli_si128(tmpsH[4 + 2], 8);

		X[0] = _mm_xor_si128(tmpsH[3], tmpsH[1]);
		Y[0] = _mm_xor_si128(tmpsH[0], tmpsH[2]);

		X[1] = _mm_xor_si128(tmpsH[4 + 3], tmpsH[4 + 1]);
		Y[1] = _mm_xor_si128(tmpsH[4 + 0], tmpsH[4 + 2]);

		addx8_1wise(inc4, ctr, ctr);

		scatter_store_n2x4((__m128i *)C + 16 * i, tmpsL);
		scatter_store_n2x4((__m128i *)C + 16 * i + 1, tmpsR);
	}
	Y[0] = polyreduce128(ctx.poly, Y[0]);
	Z[0] = _mm_xor_si128(X[0], Y[0]);
	X[0] = Z[0];

	Y[1] = polyreduce128(ctx.poly, Y[1]);
	Z[1] = _mm_xor_si128(X[1], Y[1]);
	X[1] = Z[1];

	alignas(16) __m128i W[2];

	for (size_t i = 0; i < dx4rem; i++)
	{
		gather_load_n2x1((__m128i *)M + dx4len * 16 + 4 * i, dataL);
		gather_load_n2x1((__m128i *)M + dx4len * 16 + 4 * i + 1, dataR);

		addkey256(ctx.simpira_ctx.keys, ctr, tmpsL);
		simpira_b2x1_128(ctx.simpira_ctx, tmpsL);
		addkey256(ctx.simpira_ctx.keys, tmpsL, tmpsL);
		xorx2_1wise(tmpsL, dataL, dataL);
		xorx2_1wise(S2, ctx.simpira_ctx.keys, W);
		addkey256(W, dataR, tmpsR);

		seq_graycode_n2x1(mask, omega, dx4len * 4 + i);

		simpira_b2x1_128(ctx.simpira_ctx, tmpsR);
		addkey256(ctx.simpira_ctx.keys, tmpsR, tmpsR);
		xorx2_1wise(tmpsR, mask, tmpsR);

		scatter_store_n2x1((__m128i *)C + dx4len * 16 + 4 * i, dataL);
		scatter_store_n2x1((__m128i *)C + dx4len * 16 + 4 * i + 1, tmpsR);

		X[0] = _mm_xor_si128(X[0], dataL[0]);
		X[0] = polydot128(ctx.poly, X[0], ctx.htbl[0]);
		X[0] = _mm_xor_si128(X[0], dataL[1]);
		X[0] = polydot128(ctx.poly, X[0], ctx.htbl[0]);

		X[1] = _mm_xor_si128(X[1], dataL[0]);
		X[1] = polydot128(ctx.poly, X[1], ctx.htbl[0]);
		X[1] = _mm_xor_si128(X[1], dataL[1]);
		X[1] = polydot128(ctx.poly, X[1], ctx.htbl[0]);

		addx2_1wise(inc, ctr, ctr);
	}

	if (m % 2 == 0)
	{
		dataR[0] = _mm_loadu_si128((__m128i *)(M + Mlen - 32));

		uint8_t padded[33];
		memset(padded, 0, 32);
		memcpy(padded, M + Mlen - 32 - rem, rem);
		padded[rem] = 0x80;
		dataL[0] = _mm_loadu_si128((__m128i *)padded);
		dataL[1] = _mm_loadu_si128((__m128i *)padded);

		seq_graycode_n2x1(mask, L, dlen);

		addkey256(ctx.simpira_ctx.keys, ctr, tmpsL);
		simpira_b2x16_128(ctx.simpira_ctx, tmpsL);
		addkey256(ctx.simpira_ctx.keys, tmpsL, tmpsL);
		xorx2_1wise(dataL, tmpsL, dataL);

		xorx2_1wise(S2, dataR, tmpsR);

		addkey256(ctx.simpira_ctx.keys, tmpsR, tmpsR);
		simpira_b2x16_128(ctx.simpira_ctx, tmpsR);
		addkey256(ctx.simpira_ctx.keys, tmpsR, tmpsR);
		xorx2_1wise(mask, tmpsR, dataR);

		_mm_storeu_si128((__m128i *)(C + Mlen - 32), tmpsR[0]);
		_mm_storeu_si128((__m128i *)(C + Mlen - 32) + 1, tmpsR[1]);

		_mm_storeu_si128((__m128i *)padded, dataL[0]);
		_mm_storeu_si128((__m128i *)padded + 1, dataL[1]);
		memcpy(C + Mlen - 32 - rem, padded, rem);

		X[0] = _mm_xor_si128(X[0], dataL[0]);
		X[0] = polydot128(ctx.poly, X[0], ctx.htbl[0]);
		X[0] = _mm_xor_si128(X[0], dataL[1]);
		X[0] = polydot128(ctx.poly, X[0], ctx.htbl[0]);

		X[1] = _mm_xor_si128(X[1], dataL[0]);
		X[1] = polydot128(ctx.poly, X[1], ctx.htbl[0]);
		X[1] = _mm_xor_si128(X[1], dataL[1]);
		X[1] = polydot128(ctx.poly, X[1], ctx.htbl[0]);
	}
	else
	{
		uint8_t padded[33];
		memset(padded, 0, 32);
		memcpy(padded, M + Mlen - rem, rem);
		padded[rem] = 0x80;
		dataL[0] = _mm_loadu_si128((__m128i *)padded);
		dataL[0] = _mm_loadu_si128((__m128i *)padded + 1);

		addkey256(ctx.simpira_ctx.keys, ctr, tmpsL);
		simpira_b2x16_128(ctx.simpira_ctx, tmpsL);
		addkey256(ctx.simpira_ctx.keys, tmpsL, tmpsL);
		xorx2_1wise(dataL, tmpsL, dataL);

		_mm_storeu_si128((__m128i *)padded, tmpsL[0]);
		_mm_storeu_si128((__m128i *)padded + 1, tmpsL[1]);
		memcpy(C + Mlen - 32 - rem, padded, rem);

		X[0] = _mm_xor_si128(X[0], dataL[0]);
		X[0] = polydot128(ctx.poly, X[0], ctx.htbl[0]);
		X[0] = _mm_xor_si128(X[0], dataL[1]);
		X[0] = polydot128(ctx.poly, X[0], ctx.htbl[0]);

		X[1] = _mm_xor_si128(X[1], dataL[0]);
		X[1] = polydot128(ctx.poly, X[1], ctx.htbl[0]);
		X[1] = _mm_xor_si128(X[1], dataL[1]);
		X[1] = polydot128(ctx.poly, X[1], ctx.htbl[0]);
	}
	hash[0] = X[0];
	hash[1] = X[1];
}


static inline void middlelowerx8(aespoly_context ctx, __m128i *state, __m128i *S1, __m128i *S2, const uint8_t *M, size_t Mlen, uint8_t *C, __m128i *hash)
{
	size_t m = (Mlen + 31) / 32;
	size_t rem = Mlen + 32 - m * 32;
	size_t dlen = MAX(m - 1, 0) / 2;
	size_t dx8rem = dlen % 8;
	size_t dx8len = dlen / 8;


	alignas(16) __m128i data[32];
	alignas(16) __m128i tmps[64];
	alignas(16) __m128i omegai[8];

	__m128i *dataL = data;
	__m128i *dataR = data + 16;

	__m128i *tmpsL = tmps;
	__m128i *tmpsR = tmps + 16;
	__m128i *mask = tmps + 32;
	__m128i *tmpsH = tmps + 48;

	setzero_x16(mask);
	setzero_x16(tmpsL);

	alignas(16) __m128i X[2] = {state[0], state[1]};
	alignas(16) __m128i Y[2] = {_mm_setzero_si128(), _mm_setzero_si128()};
	alignas(16) __m128i Z[2] = {_mm_setzero_si128(), _mm_setzero_si128()};

	alignas(16) __m128i ctr[16];
	alignas(16) __m128i inc8[16];
	alignas(16) __m128i inc[16];

	for (size_t i = 0; i < 4; i++)
	{
		ctr[2 * i] = _mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S1[0]);
		ctr[2 * i + 1] = _mm_xor_si128(_mm_setr_epi32(i + 1, 0, 0, 0), S1[1]);
		inc8[2 * i] = _mm_srli_si128(_mm_setr_epi32(1 << 2, 0, 0, 0), 2);
		inc8[2 * i + 1] = _mm_setr_epi32(8, 0, 0, 0);
		inc[2 * i] = _mm_srli_si128(_mm_setr_epi32(1 << 2, 0, 0, 0), 2);
		inc[2 * i + 1] = _mm_setr_epi32(1, 0, 0, 0);
	}

	if (Mlen < 32)
	{
		return;
	}

	copyx2(omega, omegai);

	for (size_t i = 0; i < dx8len; i++)
	{
		gather_load_n2x8((__m128i *)M + 32 * i, dataL);
		gather_load_n2x8((__m128i *)M + 32 * i + 1, dataR);

		seq_graycode_n2x8(mask, omegai, i);

		Y[0] = polyreduce128(ctx.poly, Y[0]);
		Y[1] = polyreduce128(ctx.poly, Y[1]);
		Z[0] = _mm_xor_si128(X[0], Y[0]);
		Z[1] = _mm_xor_si128(X[1], Y[1]);

		addkey256x8(S1, ctr, tmpsL);
		addkey256x8(ctx.simpira_ctx.keys, tmpsL, tmpsL);
		simpira_b2x8_128(ctx.simpira_ctx, tmpsL);
		addkey256x8(ctx.simpira_ctx.keys, tmpsL, tmpsL);
		xorx16_1wise(tmpsL, dataL, tmpsL);
		addkey256x8(S2, dataR, tmpsR);

		Z[0] = _mm_xor_si128(Z[0], tmpsL[0]);
		Z[1] = _mm_xor_si128(Z[1], tmpsL[0]);

		simpira_roundx8(ctx.simpira_ctx.c[1], tmpsR);
		mulinit_n2(tmpsL, ctx.htbl, tmpsH, 16);
		simpira_roundx8_rev(ctx.simpira_ctx.c[2], tmpsR);
		simpira_roundx8(ctx.simpira_ctx.c[3], tmpsR);
		muladd_n2(tmpsL, ctx.htbl, tmpsH, 16, 1);
		simpira_roundx8_rev(ctx.simpira_ctx.c[4], tmpsR);

		lower_roundx8(tmpsL, tmpsR, tmpsH, tmpsR, ctx.simpira_ctx.c, ctx.htbl, 16, 1);
		lower_roundx8(tmpsL, tmpsR, tmpsH, tmpsR, ctx.simpira_ctx.c, ctx.htbl, 16, 2);

		simpira_roundx8(ctx.simpira_ctx.c[13], tmpsR);
		muladd_n2(tmpsL, ctx.htbl, tmpsH, 16, 6);
		simpira_roundx8_rev(ctx.simpira_ctx.c[14], tmpsR);
		simpira_roundx8(ctx.simpira_ctx.c[15], tmpsR);
		muladd_n2(tmpsL, ctx.htbl, tmpsH, 16, 7);
		muladd_n2(tmpsL, ctx.htbl, tmpsH, 16, 8);
		muladd_n2(tmpsL, ctx.htbl, tmpsH, 16, 9);
		muladd_n2(tmpsL, ctx.htbl, tmpsH, 16, 10);
		muladd_n2(tmpsL, ctx.htbl, tmpsH, 16, 11);
		muladd_n2(tmpsL, ctx.htbl, tmpsH, 16, 12);
		muladd_n2(tmpsL, ctx.htbl, tmpsH, 16, 13);
		muladd_n2(tmpsL, ctx.htbl, tmpsH, 16, 14);
		muladdlast_n2(Z, ctx.htbl, tmpsH, 16);
		addkey256x8(ctx.simpira_ctx.keys, tmpsR, tmpsR);

		xorx16_1wise(tmpsR, mask, tmpsR);

		tmpsH[3] = _mm_bsrli_si128(tmpsH[2], 8);
		tmpsH[2] = _mm_bslli_si128(tmpsH[2], 8);

		tmpsH[4 + 3] = _mm_bsrli_si128(tmpsH[4 + 2], 8);
		tmpsH[4 + 2] = _mm_bslli_si128(tmpsH[4 + 2], 8);

		X[0] = _mm_xor_si128(tmpsH[3], tmpsH[1]);
		Y[0] = _mm_xor_si128(tmpsH[0], tmpsH[2]);

		X[1] = _mm_xor_si128(tmpsH[4 + 3], tmpsH[4 + 1]);
		Y[1] = _mm_xor_si128(tmpsH[4 + 0], tmpsH[4 + 2]);

		addx16_1wise(inc8, ctr, ctr);

		scatter_store_n2x8((__m128i *)C + 32 * i, tmpsL);
		scatter_store_n2x8((__m128i *)C + 32 * i + 1, tmpsR);
	}
	Y[0] = polyreduce128(ctx.poly, Y[0]);
	Z[0] = _mm_xor_si128(X[0], Y[0]);
	X[0] = Z[0];

	Y[1] = polyreduce128(ctx.poly, Y[1]);
	Z[1] = _mm_xor_si128(X[1], Y[1]);
	X[1] = Z[1];

	alignas(16) __m128i W[2];

	for (size_t i = 0; i < dx8rem; i++)
	{
		gather_load_n2x1((__m128i *)M + dx8len * 32 + 4 * i, dataL);
		gather_load_n2x1((__m128i *)M + dx8len * 32 + 4 * i + 1, dataR);

		addkey256(ctx.simpira_ctx.keys, ctr, tmpsL);
		simpira_b2x1_128(ctx.simpira_ctx, tmpsL);
		addkey256(ctx.simpira_ctx.keys, tmpsL, tmpsL);
		xorx2_1wise(tmpsL, dataL, dataL);
		xorx2_1wise(S2, ctx.simpira_ctx.keys, W);
		addkey256(W, dataR, tmpsR);

		seq_graycode_n2x1(mask, omegai, dx8len * 8 + i);

		simpira_b2x1_128(ctx.simpira_ctx, tmpsR);
		addkey256(ctx.simpira_ctx.keys, tmpsR, tmpsR);
		xorx2_1wise(tmpsR, mask, tmpsR);

		scatter_store_n2x1((__m128i *)C + dx8len * 32 + 4 * i, dataL);
		scatter_store_n2x1((__m128i *)C + dx8len * 32 + 4 * i + 1, tmpsR);

		X[0] = _mm_xor_si128(X[0], dataL[0]);
		X[0] = polydot128(ctx.poly, X[0], ctx.htbl[0]);
		X[0] = _mm_xor_si128(X[0], dataL[1]);
		X[0] = polydot128(ctx.poly, X[0], ctx.htbl[0]);

		X[1] = _mm_xor_si128(X[1], dataL[0]);
		X[1] = polydot128(ctx.poly, X[1], ctx.htbl[0]);
		X[1] = _mm_xor_si128(X[1], dataL[1]);
		X[1] = polydot128(ctx.poly, X[1], ctx.htbl[0]);

		addx2_1wise(inc, ctr, ctr);
	}

	if (m % 2 == 0)
	{
		dataR[0] = _mm_loadu_si128((__m128i *)(M + Mlen - 32));

		uint8_t padded[33];
		memset(padded, 0, 32);
		memcpy(padded, M + Mlen - 32 - rem, rem);
		padded[rem] = 0x80;
		dataL[0] = _mm_loadu_si128((__m128i *)padded);
		dataL[1] = _mm_loadu_si128((__m128i *)padded);

		seq_graycode_n2x1(mask, L, dlen);

		addkey256(ctx.simpira_ctx.keys, ctr, tmpsL);
		simpira_b2x16_128(ctx.simpira_ctx, tmpsL);
		addkey256(ctx.simpira_ctx.keys, tmpsL, tmpsL);
		xorx2_1wise(dataL, tmpsL, dataL);

		xorx2_1wise(S2, dataR, tmpsR);

		addkey256(ctx.simpira_ctx.keys, tmpsR, tmpsR);
		simpira_b2x16_128(ctx.simpira_ctx, tmpsR);
		addkey256(ctx.simpira_ctx.keys, tmpsR, tmpsR);
		xorx2_1wise(mask, tmpsR, dataR);

		_mm_storeu_si128((__m128i *)(C + Mlen - 32), tmpsR[0]);
		_mm_storeu_si128((__m128i *)(C + Mlen - 32) + 1, tmpsR[1]);

		_mm_storeu_si128((__m128i *)padded, dataL[0]);
		_mm_storeu_si128((__m128i *)padded + 1, dataL[1]);
		memcpy(C + Mlen - 32 - rem, padded, rem);

		X[0] = _mm_xor_si128(X[0], dataL[0]);
		X[0] = polydot128(ctx.poly, X[0], ctx.htbl[0]);
		X[0] = _mm_xor_si128(X[0], dataL[1]);
		X[0] = polydot128(ctx.poly, X[0], ctx.htbl[0]);

		X[1] = _mm_xor_si128(X[1], dataL[0]);
		X[1] = polydot128(ctx.poly, X[1], ctx.htbl[0]);
		X[1] = _mm_xor_si128(X[1], dataL[1]);
		X[1] = polydot128(ctx.poly, X[1], ctx.htbl[0]);
	}
	else
	{
		uint8_t padded[33];
		memset(padded, 0, 32);
		memcpy(padded, M + Mlen - rem, rem);
		padded[rem] = 0x80;
		dataL[0] = _mm_loadu_si128((__m128i *)padded);
		dataL[0] = _mm_loadu_si128((__m128i *)padded + 1);

		addkey256(ctx.simpira_ctx.keys, ctr, tmpsL);
		simpira_b2x16_128(ctx.simpira_ctx, tmpsL);
		addkey256(ctx.simpira_ctx.keys, tmpsL, tmpsL);
		xorx2_1wise(dataL, tmpsL, dataL);

		_mm_storeu_si128((__m128i *)padded, tmpsL[0]);
		_mm_storeu_si128((__m128i *)padded + 1, tmpsL[1]);
		memcpy(C + Mlen - 32 - rem, padded, rem);

		X[0] = _mm_xor_si128(X[0], dataL[0]);
		X[0] = polydot128(ctx.poly, X[0], ctx.htbl[0]);
		X[0] = _mm_xor_si128(X[0], dataL[1]);
		X[0] = polydot128(ctx.poly, X[0], ctx.htbl[0]);

		X[1] = _mm_xor_si128(X[1], dataL[0]);
		X[1] = polydot128(ctx.poly, X[1], ctx.htbl[0]);
		X[1] = _mm_xor_si128(X[1], dataL[1]);
		X[1] = polydot128(ctx.poly, X[1], ctx.htbl[0]);
	}
	hash[0] = X[0];
	hash[1] = X[1];
}


static inline void init(aespoly_context *ctx, uint8_t *key)
{
	ctx->key[0] = _mm_loadu_si128((__m128i *)key);
	ctx->key[1] = _mm_loadu_si128((__m128i *)key + 1);

	simpira_b2_init(&ctx->simpira_ctx);

	ctx->poly_double = _mm_setr_epi32(0x87, 0, 0, 0);
	ctx->poly = _mm_setr_epi32(0x1, 0, 0, 0xc2000000);
	alignas(16) __m128i tmps[4];

	tmps[0] = ctx->key[0];
	tmps[1] = ctx->key[1];
	simpira_b2_128(ctx->simpira_ctx, tmps[0], tmps[1]);
	tmps[0] = _mm_xor_si128(ctx->key[0], tmps[0]);
	tmps[1] = _mm_xor_si128(ctx->key[1], tmps[1]);
	L[0] = tmps[0];
	L[1] = tmps[1];

	tmps[0] = _mm_xor_si128(ctx->key[0], _mm_setr_epi32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff));
	tmps[1] = _mm_xor_si128(ctx->key[1], _mm_setr_epi32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff));
	simpira_b2_128(ctx->simpira_ctx, tmps[0], tmps[1]);
	tmps[0] = _mm_xor_si128(ctx->key[0], tmps[0]);
	tmps[1] = _mm_xor_si128(ctx->key[1], tmps[1]);
	L[0] = tmps[0];
	L[1] = tmps[1];
	for (size_t i = 1; i < 250; i++)
	{
		L[i] = double128(ctx->poly, L[i - 1]);
	}
	for (size_t i = 1; i < 250; i++)
	{

		omega[i] = double128(ctx->poly, omega[i - 1]);
	}
}

static inline void encx4(aespoly_context ctx, uint8_t *T, size_t Tlen, uint8_t *M, size_t Mlen, uint8_t *C)
{
	uint8_t *MN = M;
	uint8_t *MLR = M + 32;
	uint8_t *CN = C;
	uint8_t *CLR = C + 32;

	size_t MLRlen = MAX(Mlen - 32, 0);

	alignas(16) __m128i tmps[16];

	bool mln = MLRlen % 32 > 0;

	tweaker(ctx, mln, T, Tlen, tmps, tmps + 2);

	__m128i *state = tmps;
	__m128i *phash_res = tmps + 2;

	tmps[4] = _mm_loadu_si128((__m128i *)MN);
	tmps[5] = _mm_loadu_si128((__m128i *)MN);

	__m128i *mn = tmps + 4;
	__m128i *n = tmps + 4;

	n[0] = _mm_xor_si128(phash_res[0], mn[0]);
	n[1] = _mm_xor_si128(phash_res[1], mn[1]);

	__m128i *hash = tmps + 6;
	__m128i *sum = tmps + 8;

	upper(ctx, state, MLR, MLRlen, CLR, hash, sum);

	n[0] = _mm_xor_si128(n[0], hash[0]);
	n[1] = _mm_xor_si128(n[1], hash[1]);

	n[0] = _mm_xor_si128(n[0], sum[0]);
	n[1] = _mm_xor_si128(n[1], sum[1]);

	simpira_b2_128(ctx.simpira_ctx, n[0], n[1]);

	__m128i *n_prev = tmps + 12;

	n_prev[0] = n[0];
	n_prev[1] = n[1];

	middlelower(ctx, state, n, n, CLR, MLRlen, CLR, hash);

	simpira_b2_128(ctx.simpira_ctx, n[0], n[1]);

	n[0] = _mm_xor_si128(hash[0], n[0]);
	n[1] = _mm_xor_si128(hash[1], n[1]);

	n[0] = _mm_xor_si128(phash_res[0], n[0]);
	n[1] = _mm_xor_si128(phash_res[1], n[1]);

	if ((MLRlen / 32) % 4 > 0)
	{
		n[0] = _mm_xor_si128(n_prev[0], n[0]);
		n[1] = _mm_xor_si128(n_prev[1], n[1]);
	}

	_mm_storeu_si128((__m128i *)C, n[0]);
	_mm_storeu_si128((__m128i *)C + 1, n[1]);
}

static inline void encx8(aespoly_context ctx, uint8_t *T, size_t Tlen, uint8_t *M, size_t Mlen, uint8_t *C)
{
	uint8_t *MN = M;
	uint8_t *MLR = M + 32;
	uint8_t *CN = C;
	uint8_t *CLR = C + 32;

	size_t MLRlen = MAX(Mlen - 32, 0);

	alignas(16) __m128i tmps[16];

	bool mln = MLRlen % 32 > 0;

	tweaker(ctx, mln, T, Tlen, tmps, tmps + 2);

	__m128i *state = tmps;
	__m128i *phash_res = tmps + 2;

	tmps[4] = _mm_loadu_si128((__m128i *)MN);
	tmps[5] = _mm_loadu_si128((__m128i *)MN);

	__m128i *mn = tmps + 4;
	__m128i *n = tmps + 4;

	n[0] = _mm_xor_si128(phash_res[0], mn[0]);
	n[1] = _mm_xor_si128(phash_res[1], mn[1]);

	__m128i *hash = tmps + 6;
	__m128i *sum = tmps + 8;

	upperx8(ctx, state, MLR, MLRlen, CLR, hash, sum);

	n[0] = _mm_xor_si128(n[0], hash[0]);
	n[1] = _mm_xor_si128(n[1], hash[1]);

	n[0] = _mm_xor_si128(n[0], sum[0]);
	n[1] = _mm_xor_si128(n[1], sum[1]);

	simpira_b2_128(ctx.simpira_ctx, n[0], n[1]);

	__m128i *n_prev = tmps + 12;

	n_prev[0] = n[0];
	n_prev[1] = n[1];

	middlelowerx8(ctx, state, n, n, CLR, MLRlen, CLR, hash);

	simpira_b2_128(ctx.simpira_ctx, n[0], n[1]);

	n[0] = _mm_xor_si128(hash[0], n[0]);
	n[1] = _mm_xor_si128(hash[1], n[1]);

	n[0] = _mm_xor_si128(phash_res[0], n[0]);
	n[1] = _mm_xor_si128(phash_res[1], n[1]);

	if ((MLRlen / 32) % 4 > 0)
	{
		n[0] = _mm_xor_si128(n_prev[0], n[0]);
		n[1] = _mm_xor_si128(n_prev[1], n[1]);
	}

	_mm_storeu_si128((__m128i *)C, n[0]);
	_mm_storeu_si128((__m128i *)C + 1, n[1]);
}