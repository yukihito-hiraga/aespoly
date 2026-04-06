#pragma once
#include "core.h"
#include <stdlib.h>

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

static inline void init_htbl128(__m128i *htbl, __m128i poly, __m128i H)
{
	htbl[0] = H;
	for (size_t i = 1; i < 16; i++)
	{
		htbl[i] = polydot128(poly, htbl[i - 1], H);
	}
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

static inline __m128i mul2_xex(__m128i pp, __m128i X)
{
	__m128i tmp1, tmp2, tmp3, tmp4, tmp5;

	tmp1 = _mm_slli_epi32(X, 1);
	tmp2 = _mm_srli_epi32(X, 31);
	tmp3 = _mm_slli_si128(tmp2, 4);
	X = _mm_xor_si128(tmp1, tmp3);
	tmp4 = _mm_srli_si128(tmp2, 12);
	tmp5 = _mm_shuffle_epi8(pp, tmp4);
	X = _mm_xor_si128(X, tmp5);
	return X;
}

#define mul2rev_xex(pp, X) byterev(mul2_xex(pp, byterev(X)))

static inline __m128i phash(aespolyW_context ctx, const uint8_t *M, size_t Mlen)
{
	size_t blen = Mlen / 16;
	size_t brem = Mlen % 16;

	alignas(16) __m128i data[4];
	alignas(16) __m128i tmps[4];
	alignas(16) __m128i sum = _mm_setzero_si128();

	for (size_t i = 0; i < blen; i++)
	{
		data[0] = _mm_loadu_si128((__m128i *)M + i);
		tmps[0] = _mm_xor_si128(data[0], ctx.L[i + 1]);
		tmps[0] = aesenc128(tmps[0], ctx.aes_ctx.keys128);
		sum = _mm_xor_si128(sum, tmps[0]);
	}

	if (brem)
	{
		uint8_t padded[16];
		memset(padded, 0, 16);
		memcpy(padded, M + 16 * blen, brem);
		padded[brem] = 0x80;
		alignas(16) __m128i blk = _mm_loadu_si128((__m128i *)padded);
		blk = _mm_xor_si128(blk, ctx.L[blen + 1]);
		blk = aesenc128(blk, ctx.aes_ctx.keys128);
		sum = _mm_xor_si128(sum, blk);
	}

	return sum;
}

static inline __m128i tweaker_scalar(aespolyW_context ctx, bool mln,
									 const uint8_t *T, size_t Tlen)
{
	__m128i X =
		_mm_setr_epi64(_m_from_int64(16 * Tlen + 2 + (mln ? 1 : 0)),
					   _m_from_int64(0));
	X = polydot128(ctx.poly, X, ctx.htbl[0]);

	size_t blen = Tlen / 16;
	size_t rem = Tlen % 16;
	for (size_t i = 0; i < blen; i++)
	{
		__m128i blk = _mm_loadu_si128((__m128i *)T + i);
		X = _mm_xor_si128(X, blk);
		X = polydot128(ctx.poly, X, ctx.htbl[0]);
	}

	if (rem > 0)
	{
		uint8_t padded[16];
		memset(padded, 0, 16);
		memcpy(padded, T + 16 * blen, rem);
		__m128i blk = _mm_loadu_si128((__m128i *)padded);
		X = _mm_xor_si128(X, blk);
		X = polydot128(ctx.poly, X, ctx.htbl[0]);
	}

	return X;
}

#define tweaker_round(dataR, dataL, tmpsR, tmpsL, keys, mask, S, htbl, n, i) \
	{                                                                        \
		tmpsR[i] = _mm_xor_si128(dataR[i], mask[i]);                         \
		tmpsR[i] = aesenc128(tmpsR[i], keys);                                \
		S = _mm_xor_si128(tmpsR[i], S);                                      \
		schoolbook_add128(dataL[n - 1 - i], htbl[i], tmps);                  \
	}

static inline void tweaker(aespolyW_context ctx, bool mln, const uint8_t *T, size_t Tlen, __m128i *state, __m128i *hash)
{
	size_t m = (Tlen + 15) / 16;
	size_t rem = Tlen + 16 - m * 16;
	size_t dlen = MAX(m - m % 2 - 2, 0) / 2;
	size_t dx8rem = dlen % 8;
	size_t dx8len = dlen / 8;

	alignas(16) __m128i data[16];
	alignas(16) __m128i tmps[24];

	__m128i *dataL = data;
	__m128i *dataR = data + 8;

	__m128i *tmpsL = tmps;
	__m128i *tmpsR = tmps + 8;
	__m128i *mask = tmps + 16;

	setzero_x8(mask);

	alignas(16) __m128i X = _mm_setr_epi64(_m_from_int64(16 * Tlen + 2 + mln), _m_from_int64(0));
	X = polydot128(ctx.poly, X, ctx.htbl[0]);
	alignas(16) __m128i Y = _mm_setzero_si128(), Z = _mm_setzero_si128();

	if (Tlen < 16)
	{
		state[0] = X;
		hash[0] = _mm_setzero_si128();
		return;
	}

	alignas(16) __m128i sum = _mm_setzero_si128();
	alignas(16) __m128i Li = ctx.L[3];

	for (size_t i = 0; i < dx8len; i++)
	{
		gather_load_x8((__m128i *)T + 16 * i + 1, dataL);
		gather_load_x8((__m128i *)T + 16 * i, dataR);

		seq_graycode_x8(mask, ctx.L, ctx.L[2], Li, i);

		Y = polyreduce128(ctx.poly, Y);
		Z = _mm_xor_si128(X, Y);
		Z = _mm_xor_si128(Z, dataL[0]);

		tmpsR[0] = _mm_xor_si128(dataR[0], mask[0]);
		tmpsR[0] = aesenc128(tmpsR[0], ctx.aes_ctx.keys128);
		sum = _mm_xor_si128(tmpsR[0], sum);
		schoolbook_initialadd128(dataL[7], ctx.htbl[0], tmps);

		tweaker_round(dataR, dataL, tmpsR, tmpsL, ctx.aes_ctx.keys128, mask, sum, ctx.htbl, 8, 1);
		tweaker_round(dataR, dataL, tmpsR, tmpsL, ctx.aes_ctx.keys128, mask, sum, ctx.htbl, 8, 2);
		tweaker_round(dataR, dataL, tmpsR, tmpsL, ctx.aes_ctx.keys128, mask, sum, ctx.htbl, 8, 3);
		tweaker_round(dataR, dataL, tmpsR, tmpsL, ctx.aes_ctx.keys128, mask, sum, ctx.htbl, 8, 4);
		tweaker_round(dataR, dataL, tmpsR, tmpsL, ctx.aes_ctx.keys128, mask, sum, ctx.htbl, 8, 5);
		tweaker_round(dataR, dataL, tmpsR, tmpsL, ctx.aes_ctx.keys128, mask, sum, ctx.htbl, 8, 6);

		tmpsR[7] = _mm_xor_si128(dataR[7], mask[7]);
		tmpsR[7] = aesenc128(tmpsR[7], ctx.aes_ctx.keys128);
		sum = _mm_xor_si128(tmpsR[7], sum);
		schoolbook_add128(Z, ctx.htbl[7], tmps);

		tmpsL[3] = _mm_bsrli_si128(tmpsL[2], 8);
		tmpsL[2] = _mm_bslli_si128(tmpsL[2], 8);

		X = _mm_xor_si128(tmpsL[3], tmpsL[1]);
		Y = _mm_xor_si128(tmpsL[0], tmpsL[2]);
	}
	Y = polyreduce128(ctx.poly, Y);
	Z = _mm_xor_si128(X, Y);
	X = Z;
	for (size_t i = 0; i < dx8rem; i++)
	{
		gather_load_n2x1((__m128i *)T + dx8len * 16 + 2 * i, dataR);
		gather_load_n2x1((__m128i *)T + dx8len * 16 + 2 * i + 1, dataL);

		seq_graycode_x1(mask, ctx.L, dx8len * 8 + i);

		tmpsR[0] = _mm_xor_si128(dataR[0], mask[0]);
		tmpsR[0] = aesenc128(tmpsR[0], ctx.aes_ctx.keys128);
		sum = _mm_xor_si128(tmpsR[0], sum);

		X = _mm_xor_si128(X, dataL[0]);
		X = polydot128(ctx.poly, X, ctx.htbl[0]);
	}

	if (Tlen == 16)
	{
		state[0] = X;
		hash[0] = _mm_loadu_si128((__m128i *)T);
	}
	else
	{
		if (m % 2 == 0)
		{
			data[0] = _mm_loadu_si128((__m128i *)T + dlen * 2);

			uint8_t padded[16];
			memset(padded, 0, 16);
			memcpy(padded, T + Tlen - rem, rem);
			// padded[rem] = 0x80;
			alignas(16) __m128i paddedblk = _mm_loadu_si128((__m128i *)padded);

			seq_graycode_x1(mask, ctx.L, dlen);

			tmps[0] = _mm_xor_si128(data[0], mask[0]);
			tmps[0] = aesenc128(tmps[0], ctx.aes_ctx.keys128);
			hash[0] = _mm_xor_si128(tmps[0], sum);

			X = _mm_xor_si128(X, paddedblk);
			state[0] = polydot128(ctx.poly, X, ctx.htbl[0]);
		}
		else
		{
			data[0] = _mm_loadu_si128((__m128i *)T + dlen * 2);
			data[2] = _mm_loadu_si128((__m128i *)(T + Tlen - 16));

			uint8_t padded[16];
			memset(padded, 0, 16);
			memcpy(padded, T + Tlen - 16 - rem, rem);
			// padded[rem] = 0x80;
			alignas(16) __m128i paddedblk = _mm_loadu_si128((__m128i *)padded);

			seq_graycode_x1(mask, ctx.L, dlen);

			tmps[0] = _mm_xor_si128(data[0], mask[0]);
			tmps[0] = aesenc128(tmps[0], ctx.aes_ctx.keys128);
			sum = _mm_xor_si128(tmps[0], sum);

			seq_graycode_x1(mask, ctx.L, dlen + 1);
			tmps[0] = _mm_xor_si128(data[2], mask[0]);
			tmps[0] = aesenc128(tmps[0], ctx.aes_ctx.keys128);
			sum = _mm_xor_si128(tmps[0], sum);

			X = _mm_xor_si128(X, paddedblk);
			state[0] = polydot128(ctx.poly, X, ctx.htbl[0]);
			hash[0] = _mm_xor_si128(sum, data[2]);
		}
	}
}

#define upper_round(dataR, dataL, tmpsR, tmpsL, keys, mask, S, htbl, n, i) \
	{                                                                      \
		tmpsR[i] = _mm_xor_si128(dataR[i], mask[i]);                       \
		tmpsR[i] = aesenc128(tmpsR[i], keys);                              \
		S = _mm_xor_si128(tmpsR[i], S);                                    \
		schoolbook_add128(dataL[n - 1 - i], htbl[i], tmpsL);               \
	}

static inline void upper(aespolyW_context ctx, __m128i state, const uint8_t *M, size_t Mlen, uint8_t *C, __m128i *hash, __m128i *sum)
{
	size_t m = (Mlen + 15) / 16;
	size_t rem = Mlen + 16 - m * 16;
	size_t dlen = MAX(m - 1, 0) / 2;
	size_t dx4rem = dlen % 4;
	size_t dx4len = dlen / 4;

	alignas(16) __m128i data[16];
	alignas(16) __m128i tmps[24];
	alignas(16) __m128i omegai;

	__m128i *dataL = data;
	__m128i *dataR = data + 8;

	__m128i *tmpsL = tmps;
	__m128i *tmpsR = tmps + 8;
	__m128i *mask = tmps + 16;

	setzero_x8(mask);

	alignas(16) __m128i X = state;
	alignas(16) __m128i Y = _mm_setzero_si128(), Z = _mm_setzero_si128();

	alignas(16) __m128i S = _mm_setzero_si128();

	if (Mlen < 16)
	{
		uint8_t padded[16];
		memset(padded, 0, 16);
		memcpy(padded, M, Mlen);
		padded[Mlen] = 0x01;
		hash[0] = _mm_xor_si128(state, _mm_loadu_si128((__m128i *)padded));
		hash[0] = polydot128(ctx.poly, hash[0], ctx.htbl[0]);
		sum[0] = _mm_setzero_si128();
		return;
	}

	omegai = ctx.omega[3];

	for (size_t i = 0; i < dx4len; i++)
	{
		gather_load_x4((__m128i *)M + 8 * i, dataL);
		gather_load_x4((__m128i *)M + 8 * i + 1, dataR);

		seq_graycode_x4(mask, ctx.omega, ctx.omega[1], omegai, i);

		Y = polyreduce128(ctx.poly, Y);
		Z = _mm_xor_si128(X, Y);
		Z = _mm_xor_si128(Z, dataL[0]);

		tmpsR[0] = _mm_xor_si128(dataR[0], mask[0]);
		tmpsR[0] = aesenc128(tmpsR[0], ctx.aes_ctx.keys128);
		S = _mm_xor_si128(tmpsR[0], S);
		schoolbook_initialadd128(dataL[3], ctx.htbl[0], tmpsL);

		upper_round(dataR, dataL, tmpsR, tmpsL, ctx.aes_ctx.keys128, mask, S, ctx.htbl, 4, 1);
		upper_round(dataR, dataL, tmpsR, tmpsL, ctx.aes_ctx.keys128, mask, S, ctx.htbl, 4, 2);

		tmpsR[3] = _mm_xor_si128(dataR[3], mask[3]);
		tmpsR[3] = aesenc128(tmpsR[3], ctx.aes_ctx.keys128);
		S = _mm_xor_si128(tmpsR[3], S);
		schoolbook_add128(Z, ctx.htbl[3], tmpsL);

		scatter_store_x4((__m128i *)C + 8 * i + 1, tmpsR);

		tmpsL[3] = _mm_bsrli_si128(tmpsL[2], 8);
		tmpsL[2] = _mm_bslli_si128(tmpsL[2], 8);

		X = _mm_xor_si128(tmpsL[3], tmpsL[1]);
		Y = _mm_xor_si128(tmpsL[0], tmpsL[2]);
	}
	Y = polyreduce128(ctx.poly, Y);
	Z = _mm_xor_si128(X, Y);
	X = Z;

	for (size_t i = 0; i < dx4rem; i++)
	{
		data[0] = _mm_loadu_si128((__m128i *)M + dx4len * 8 + 2 * i);
		data[1] = _mm_loadu_si128((__m128i *)M + dx4len * 8 + 2 * i + 1);

		seq_graycode_x1(mask, ctx.omega, dx4len * 4 + i);

		tmps[0] = _mm_xor_si128(data[1], mask[0]);
		tmps[0] = aesenc128(tmps[0], ctx.aes_ctx.keys128);
		S = _mm_xor_si128(tmps[0], S);

		_mm_storeu_si128((__m128i *)C + dx4len * 8 + 2 * i + 1, tmps[0]);
		X = _mm_xor_si128(X, data[0]);
		X = polydot128(ctx.poly, X, ctx.htbl[0]);
	}

	if (m % 2 == 0)
	{
		data[1] = _mm_loadu_si128((__m128i *)(M + Mlen - 16));

		uint8_t padded[17];
		memset(padded, 0, 16);
		memcpy(padded, M + Mlen - 16 - rem, rem);
		padded[rem] = 0x01;
		alignas(16) __m128i paddedblk = _mm_loadu_si128((__m128i *)padded);

		seq_graycode_x1(mask, ctx.omega, dlen);

		tmps[0] = _mm_xor_si128(data[1], mask[0]);
		tmps[0] = aesenc128(tmps[0], ctx.aes_ctx.keys128);
		S = _mm_xor_si128(tmps[0], S);

		_mm_storeu_si128((__m128i *)(C + Mlen - 16), tmps[0]);
		sum[0] = S;
		X = _mm_xor_si128(X, paddedblk);
		hash[0] = polydot128(ctx.poly, X, ctx.htbl[0]);
	}
	else
	{
		uint8_t padded[17];
		memset(padded, 0, 16);
		memcpy(padded, M + Mlen - rem, rem);
		padded[rem] = 0x01;
		alignas(16) __m128i paddedblk = _mm_loadu_si128((__m128i *)padded);

		X = _mm_xor_si128(X, paddedblk);
		hash[0] = polydot128(ctx.poly, X, ctx.htbl[0]);
		sum[0] = S;
	}
}

static inline void upperx8(aespolyW_context ctx, __m128i state, const uint8_t *M, size_t Mlen, uint8_t *C, __m128i *hash, __m128i *sum)
{
	size_t m = (Mlen + 15) / 16;
	size_t rem = Mlen + 16 - m * 16;
	size_t dlen = MAX(m - 1, 0) / 2;
	size_t dx8rem = dlen % 8;
	size_t dx8len = dlen / 8;

	alignas(16) __m128i data[16];
	alignas(16) __m128i tmps[24];
	alignas(16) __m128i omegai;

	__m128i *dataL = data;
	__m128i *dataR = data + 8;

	__m128i *tmpsL = tmps;
	__m128i *tmpsR = tmps + 8;
	__m128i *mask = tmps + 16;

	setzero_x8(mask);

	alignas(16) __m128i X = state;
	alignas(16) __m128i Y = _mm_setzero_si128(), Z = _mm_setzero_si128();

	alignas(16) __m128i S = _mm_setzero_si128();

	if (Mlen < 16)
	{
		uint8_t padded[16];
		memset(padded, 0, 16);
		memcpy(padded, M, Mlen);
		padded[Mlen] = 0x01;
		hash[0] = _mm_xor_si128(state, _mm_loadu_si128((__m128i *)padded));
		hash[0] = polydot128(ctx.poly, hash[0], ctx.htbl[0]);
		sum[0] = _mm_setzero_si128();
		return;
	}

	omegai = ctx.omega[0];

	for (size_t i = 0; i < dx8len; i++)
	{
		gather_load_x8((__m128i *)M + 16 * i, dataL);
		gather_load_x8((__m128i *)M + 16 * i + 1, dataR);

		seq_graycode_x8(mask, ctx.omega, ctx.omega[2], omegai, i);

		Y = polyreduce128(ctx.poly, Y);
		Z = _mm_xor_si128(X, Y);
		Z = _mm_xor_si128(Z, dataL[0]);

		tmpsR[0] = _mm_xor_si128(dataR[0], mask[0]);
		tmpsR[0] = aesenc128(tmpsR[0], ctx.aes_ctx.keys128);
		S = _mm_xor_si128(tmpsR[0], S);
		schoolbook_initialadd128(dataL[7], ctx.htbl[0], tmpsL);

		upper_round(dataR, dataL, tmpsR, tmpsL, ctx.aes_ctx.keys128, mask, S, ctx.htbl, 8, 1);
		upper_round(dataR, dataL, tmpsR, tmpsL, ctx.aes_ctx.keys128, mask, S, ctx.htbl, 8, 2);
		upper_round(dataR, dataL, tmpsR, tmpsL, ctx.aes_ctx.keys128, mask, S, ctx.htbl, 8, 3);
		upper_round(dataR, dataL, tmpsR, tmpsL, ctx.aes_ctx.keys128, mask, S, ctx.htbl, 8, 4);
		upper_round(dataR, dataL, tmpsR, tmpsL, ctx.aes_ctx.keys128, mask, S, ctx.htbl, 8, 5);
		upper_round(dataR, dataL, tmpsR, tmpsL, ctx.aes_ctx.keys128, mask, S, ctx.htbl, 8, 6);

		tmpsR[7] = _mm_xor_si128(dataR[7], mask[7]);
		tmpsR[7] = aesenc128(tmpsR[7], ctx.aes_ctx.keys128);
		S = _mm_xor_si128(tmpsR[7], S);
		schoolbook_add128(Z, ctx.htbl[7], tmpsL);

		scatter_store_x8((__m128i *)C + 16 * i + 1, tmpsR);

		tmpsL[3] = _mm_bsrli_si128(tmpsL[2], 8);
		tmpsL[2] = _mm_bslli_si128(tmpsL[2], 8);

		X = _mm_xor_si128(tmpsL[3], tmpsL[1]);
		Y = _mm_xor_si128(tmpsL[0], tmpsL[2]);
	}
	Y = polyreduce128(ctx.poly, Y);
	Z = _mm_xor_si128(X, Y);
	X = Z;

	for (size_t i = 0; i < dx8rem; i++)
	{
		data[0] = _mm_loadu_si128((__m128i *)M + dx8len * 8 + 2 * i);
		data[1] = _mm_loadu_si128((__m128i *)M + dx8len * 8 + 2 * i + 1);

		seq_graycode_x1(mask, ctx.omega, dx8len * 4 + i);

		tmps[0] = _mm_xor_si128(data[1], mask[0]);
		tmps[0] = aesenc128(tmps[0], ctx.aes_ctx.keys128);
		S = _mm_xor_si128(tmps[0], S);

		_mm_storeu_si128((__m128i *)C + dx8len * 8 + 2 * i + 1, tmps[0]);
		X = _mm_xor_si128(X, data[0]);
		X = polydot128(ctx.poly, X, ctx.htbl[0]);
	}

	if (m % 2 == 0)
	{
		data[1] = _mm_loadu_si128((__m128i *)(M + Mlen - 16));

		uint8_t padded[17];
		memset(padded, 0, 16);
		memcpy(padded, M + Mlen - 16 - rem, rem);
		padded[rem] = 0x01;
		alignas(16) __m128i paddedblk = _mm_loadu_si128((__m128i *)padded);

		seq_graycode_x1(mask, ctx.omega, dlen);

		tmps[0] = _mm_xor_si128(data[1], mask[0]);
		tmps[0] = aesenc128(tmps[0], ctx.aes_ctx.keys128);
		S = _mm_xor_si128(tmps[0], S);

		_mm_storeu_si128((__m128i *)(C + Mlen - 16), tmps[0]);
		sum[0] = S;
		X = _mm_xor_si128(X, paddedblk);
		hash[0] = polydot128(ctx.poly, X, ctx.htbl[0]);
	}
	else
	{
		uint8_t padded[17];
		memset(padded, 0, 16);
		memcpy(padded, M + Mlen - rem, rem);
		padded[rem] = 0x01;
		alignas(16) __m128i paddedblk = _mm_loadu_si128((__m128i *)padded);

		X = _mm_xor_si128(X, paddedblk);
		hash[0] = polydot128(ctx.poly, X, ctx.htbl[0]);
		sum[0] = S;
	}
}

#define middle_round(dataR, dataL, tmpsR, tmpsL, keys, ctr, S1, S2, i) \
	{                                                                  \
		(tmpsL)[i] = _mm_xor_si128((ctr)[i], S1);                      \
		(tmpsL)[i] = aesenc128((tmpsL)[i], keys);                        \
		(tmpsL)[i] = _mm_xor_si128((dataL)[i], (tmpsL)[i]);            \
		(tmpsR)[i] = _mm_xor_si128((dataR)[i], S2);                    \
	}

static inline void middle(aespolyW_context ctx, __m128i S1, __m128i S2, const uint8_t *M, size_t Mlen, uint8_t *C)
{
	size_t m = (Mlen + 15) / 16;
	size_t rem = Mlen + 16 - m * 16;
	size_t dlen = MAX(m - m % 2 - 2, 0) / 2;

	alignas(16) __m128i data[16];
	alignas(16) __m128i tmps[16];

	__m128i *dataL = data;
	__m128i *dataR = data + 8;

	__m128i *tmpsL = tmps;
	__m128i *tmpsR = tmps + 8;

	alignas(16) __m128i inc = _mm_setr_epi32(1, 0, 0, 0);
	alignas(16) __m128i ctr[1] = {
		_mm_add_epi64(inc, S1)};

	for (size_t i = 0; i < dlen; i++)
	{
		gather_load_x1((__m128i *)M + 2 * i, dataL);
		gather_load_x1((__m128i *)M + 2 * i + 1, dataR);

		middle_round(dataR, dataL, tmpsR, tmpsL, ctx.aes_ctx.keys128, ctr, S1, S2, 0);

		scatter_store_x1((__m128i *)C + 2 * i, tmpsL);
		ctr[0] = _mm_add_epi64(ctr[0], inc);
	}

	if (m % 2 == 0)
	{
		data[1] = _mm_loadu_si128((__m128i *)(M + Mlen - 16));

		uint8_t padded[17];
		memset(padded, 0, 16);
		memcpy(padded, M + Mlen - 16 - rem, rem);
		padded[rem] = 0x01;
		alignas(16) __m128i paddedblk = _mm_loadu_si128((__m128i *)padded);

		tmps[0] = _mm_xor_si128(ctr[0], S1);
		tmps[0] = aesenc128(tmps[0], ctx.aes_ctx.keys128);
		tmps[0] = _mm_xor_si128(paddedblk, tmps[0]);
		tmps[1] = _mm_xor_si128(data[1], S2);
		_mm_storeu_si128((__m128i *)padded, tmps[0]);
		_mm_storeu_si128((__m128i *)(C + Mlen - 16), tmps[1]);
		memcpy(C + Mlen - rem, padded, rem);
	}
	else
	{
		uint8_t padded[17];
		memset(padded, 0, 16);
		memcpy(padded, M + Mlen - rem, rem);
		padded[rem] = 0x01;
		alignas(16) __m128i paddedblk = _mm_loadu_si128((__m128i *)padded);

		tmps[0] = _mm_xor_si128(ctr[0], S1);
		tmps[0] = aesenc128(tmps[0], ctx.aes_ctx.keys128);
		tmps[0] = _mm_xor_si128(paddedblk, tmps[0]);
		_mm_storeu_si128((__m128i *)padded, tmps[0]);
		memcpy(C + Mlen - rem, padded, rem);
	}
}

#define lower_round(dataR, dataL, tmpsR, tmpsL, keys, mask, htbl, n, i) \
	{                                                                   \
		tmpsR[i] = aesenc128(dataR[i], keys);                           \
		tmpsR[i] = _mm_xor_si128(tmpsR[i], mask[i]);                    \
		schoolbook_add128(dataL[n - 1 - i], htbl[i], tmpsL);            \
	}

static inline __m128i lower(aespolyW_context ctx, __m128i state, const uint8_t *M, size_t Mlen, uint8_t *C)
{
	size_t m = (Mlen + 15) / 16;
	size_t rem = Mlen + 16 - m * 16;
	size_t dlen = MAX(m - 1, 0) / 2;
	size_t dx8rem = dlen % 8;
	size_t dx8len = dlen / 8;

	alignas(16) __m128i data[16];
	alignas(16) __m128i tmps[24];

	__m128i *dataL = data;
	__m128i *dataR = data + 8;

	__m128i *tmpsL = tmps;
	__m128i *tmpsR = tmps + 8;
	__m128i *mask = tmps + 16;
	alignas(16) __m128i omegai;

	setzero_x8(mask);

	alignas(16) __m128i X = state;
	alignas(16) __m128i Y = _mm_setzero_si128(), Z = _mm_setzero_si128();

	omegai = ctx.omega[3];
	for (size_t i = 0; i < dx8len; i++)
	{
		gather_load_x8((__m128i *)M + 16 * i, dataL);
		gather_load_x8((__m128i *)M + 16 * i + 1, dataR);

			seq_graycode_x8(mask, ctx.omega, ctx.omega[2], omegai, i);

		Y = polyreduce128(ctx.poly, Y);
		Z = _mm_xor_si128(X, Y);
		Z = _mm_xor_si128(Z, dataL[0]);

		tmpsR[0] = aesenc128(dataR[0], ctx.aes_ctx.keys128);
		tmpsR[0] = _mm_xor_si128(tmpsR[0], mask[0]);
		schoolbook_initialadd128(dataL[7], ctx.htbl[0], tmpsL);

		lower_round(dataR, dataL, tmpsR, tmpsL, ctx.aes_ctx.keys128, mask, ctx.htbl, 8, 1);
		lower_round(dataR, dataL, tmpsR, tmpsL, ctx.aes_ctx.keys128, mask, ctx.htbl, 8, 2);
		lower_round(dataR, dataL, tmpsR, tmpsL, ctx.aes_ctx.keys128, mask, ctx.htbl, 8, 3);
		lower_round(dataR, dataL, tmpsR, tmpsL, ctx.aes_ctx.keys128, mask, ctx.htbl, 8, 4);
		lower_round(dataR, dataL, tmpsR, tmpsL, ctx.aes_ctx.keys128, mask, ctx.htbl, 8, 5);
		lower_round(dataR, dataL, tmpsR, tmpsL, ctx.aes_ctx.keys128, mask, ctx.htbl, 8, 6);

		tmpsR[7] = aesenc128(dataR[7], ctx.aes_ctx.keys128);
		tmpsR[7] = _mm_xor_si128(tmpsR[7], mask[7]);
		schoolbook_add128(Z, ctx.htbl[7], tmpsL);

		tmpsL[3] = _mm_bsrli_si128(tmpsL[2], 8);
		tmpsL[2] = _mm_bslli_si128(tmpsL[2], 8);

		X = _mm_xor_si128(tmpsL[3], tmpsL[1]);
		Y = _mm_xor_si128(tmpsL[0], tmpsL[2]);

		scatter_store_x8((__m128i *)C + 16 * i + 1, tmpsR);
	}
	Y = polyreduce128(ctx.poly, Y);
	Z = _mm_xor_si128(X, Y);
	X = Z;

	for (size_t i = 0; i < dx8rem; i++)
	{
		data[0] = _mm_loadu_si128((__m128i *)M + dx8len * 16 + 2 * i);
		data[1] = _mm_loadu_si128((__m128i *)M + dx8len * 16 + 2 * i + 1);

		seq_graycode_x1(mask, ctx.omega, dx8len * 8 + i);

		tmps[0] = aesenc128(data[1], ctx.aes_ctx.keys128);
		tmps[0] = _mm_xor_si128(tmps[0], mask[0]);

		_mm_storeu_si128((__m128i *)C + dx8len * 16 + 2 * i, tmps[4]);
		X = _mm_xor_si128(X, data[0]);
		X = polydot128(ctx.poly, X, ctx.htbl[0]);
	}

	if (m % 2 == 0)
	{
		data[1] = _mm_loadu_si128((__m128i *)(M + Mlen - 16));

		uint8_t padded[17];
		memset(padded, 0, 16);
		memcpy(padded, M + Mlen - 16 - rem, rem);
		padded[rem] = 0x01;
		alignas(16) __m128i paddedblk = _mm_loadu_si128((__m128i *)padded);

		seq_graycode_x1(mask, ctx.omega, dlen);

		tmps[0] = aesenc128(data[1], ctx.aes_ctx.keys128);
		tmps[0] = _mm_xor_si128(tmps[0], mask[0]);

		_mm_storeu_si128((__m128i *)(C + Mlen - 16), tmps[4]);
		X = _mm_xor_si128(X, paddedblk);
		return polydot128(ctx.poly, X, ctx.htbl[0]);
	}
	else
	{
		uint8_t padded[17];
		memset(padded, 0, 16);
		memcpy(padded, M + Mlen - rem, rem);
		padded[rem] = 0x01;
		alignas(16) __m128i paddedblk = _mm_loadu_si128((__m128i *)padded);

		X = _mm_xor_si128(X, paddedblk);
		return polydot128(ctx.poly, X, ctx.htbl[0]);
	}
}

static inline __m128i middlelower(aespolyW_context* ctx, __m128i state, __m128i S1, __m128i S2, const uint8_t *M, size_t Mlen, uint8_t *C)
{
	size_t m = (Mlen + 15) / 16;
	size_t rem = Mlen + 16 - m * 16;
	size_t dlen = MAX(m - 1, 0) / 2;
	size_t dx4rem = dlen % 4;
	size_t dx4len = dlen / 4;

	alignas(16) __m128i data[16];
	alignas(16) __m128i tmps[24];
	alignas(16) __m128i omegai;

	__m128i *dataL = data;
	__m128i *dataR = data + 8;

	__m128i *tmpsL = tmps;
	__m128i *tmpsR = tmps + 8;
	__m128i *mask = tmps + 16;

	setzero_x4(mask);

	alignas(16) __m128i ctr[4] = {
		_mm_setr_epi32(1, 0, 0, 0),
		_mm_setr_epi32(2, 0, 0, 0),
		_mm_setr_epi32(3, 0, 0, 0),
		_mm_setr_epi32(4, 0, 0, 0),
	};
	alignas(16) __m128i inc4 = _mm_setr_epi32(4, 0, 0, 0);
	alignas(16) __m128i inc = _mm_setr_epi32(1, 0, 0, 0);

	alignas(16) __m128i X = state;
	alignas(16) __m128i Y = _mm_setzero_si128(), Z = _mm_setzero_si128();

	omegai = ctx->omega[3];

	for (size_t i = 0; i < dx4len; i++)
	{
		gather_load_x4((__m128i *)M + 8 * i, dataL);
		gather_load_x4((__m128i *)M + 8 * i + 1, dataR);

		seq_graycode_x4(mask, ctx->omega, ctx->omega[1], omegai, i);

		Y = polyreduce128(ctx->poly, Y);
		Z = _mm_xor_si128(X, Y);

		middle_round(dataR, dataL, tmpsR, tmpsL, ctx->aes_ctx.keys128, ctr, S1, S2, 0);
		middle_round(dataR, dataL, tmpsR, tmpsL, ctx->aes_ctx.keys128, ctr, S1, S2, 1);
		middle_round(dataR, dataL, tmpsR, tmpsL, ctx->aes_ctx.keys128, ctr, S1, S2, 2);
		middle_round(dataR, dataL, tmpsR, tmpsL, ctx->aes_ctx.keys128, ctr, S1, S2, 3);

		copyx4(tmpsL, dataL);

		Z = _mm_xor_si128(Z, dataL[0]);

		tmpsR[0] = aesenc128(tmpsR[0], ctx->aes_ctx.keys128);
		tmpsR[0] = _mm_xor_si128(tmpsR[0], mask[0]);
		schoolbook_initialadd128(dataL[3], ctx->htbl[0], tmpsL);

		lower_round(tmpsR, dataL, tmpsR, tmpsL, ctx->aes_ctx.keys128, mask, ctx->htbl, 4, 1);
		lower_round(tmpsR, dataL, tmpsR, tmpsL, ctx->aes_ctx.keys128, mask, ctx->htbl, 4, 2);

		tmpsR[3] = aesenc128(tmpsR[3], ctx->aes_ctx.keys128);
		tmpsR[3] = _mm_xor_si128(tmpsR[3], mask[3]);
		schoolbook_add128(Z, ctx->htbl[3], tmpsL);

		scatter_store_x4((__m128i *)C + 8 * i, dataL);
		scatter_store_x4((__m128i *)C + 8 * i + 1, tmpsR);

		tmpsL[3] = _mm_bsrli_si128(tmpsL[2], 8);
		tmpsL[2] = _mm_bslli_si128(tmpsL[2], 8);

		X = _mm_xor_si128(tmpsL[3], tmpsL[1]);
		Y = _mm_xor_si128(tmpsL[0], tmpsL[2]);

		addx4_bfix(ctr, inc4, ctr);
	}
	Y = polyreduce128(ctx->poly, Y);
	Z = _mm_xor_si128(X, Y);
	X = Z;

	for (size_t i = 0; i < dx4rem; i++)
	{
		gather_load_x1((__m128i *)M + dx4len * 8 + 2 * i, dataL);
		gather_load_x1((__m128i *)M + dx4len * 8 + 2 * i + 1, dataR);

		middle_round(dataR, dataL, tmpsR, tmpsL, ctx->aes_ctx.keys128, ctr, S1, S2, 0);

		seq_graycode_x1(mask, ctx->omega, dx4len * 4 + i);

		tmpsR[0] = aesenc128(tmpsR[0], ctx->aes_ctx.keys128);
		tmpsR[0] = _mm_xor_si128(tmpsR[0], mask[0]);

		scatter_store_x1((__m128i *)C + dx4len * 8 + 2 * i, tmpsL);
		scatter_store_x1((__m128i *)C + dx4len * 8 + 2 * i + 1, tmpsR);

		X = _mm_xor_si128(X, tmpsL[0]);
		X = polydot128(ctx->poly, X, ctx->htbl[0]);
		ctr[0] = _mm_add_epi64(ctr[0], inc);
	}

	if (m % 2 == 0)
	{
		dataR[0] = _mm_loadu_si128((__m128i *)(M + Mlen - 16));

		uint8_t padded[17];
		uint8_t hashblk[17];
		memset(padded, 0, 16);
		memset(hashblk, 0, 16);
		memcpy(padded, M + Mlen - 16 - rem, rem);
		dataL[0] = _mm_loadu_si128((__m128i *)padded);

		middle_round(dataR, dataL, tmpsR, tmpsL, ctx->aes_ctx.keys128, ctr, S1, S2, 0);
		seq_graycode_x1(mask, ctx->omega, dlen);

		tmpsR[0] = aesenc128(tmpsR[0], ctx->aes_ctx.keys128);
		tmpsR[0] = _mm_xor_si128(tmpsR[0], mask[0]);

		_mm_storeu_si128((__m128i *)(C + Mlen - 16), tmpsR[0]);

		_mm_storeu_si128((__m128i *)padded, tmpsL[0]);
		memcpy(C + Mlen - rem, padded, rem);
		memcpy(hashblk, C + Mlen - rem, rem);
		hashblk[rem] = 0x01;

		X = _mm_xor_si128(X, _mm_loadu_si128((__m128i *)hashblk));
		return polydot128(ctx->poly, X, ctx->htbl[0]);
	}
	else
	{
		uint8_t padded[17];
		uint8_t hashblk[17];
		memset(padded, 0, 16);
		memset(hashblk, 0, 16);
		memcpy(padded, M + Mlen - rem, rem);
		dataL[0] = _mm_loadu_si128((__m128i *)padded);

		(tmpsL)[0] = _mm_xor_si128((ctr)[0], S1);
		(tmpsL)[0] = aesenc128((tmpsL)[0], ctx->aes_ctx.keys128);
		(tmpsL)[0] = _mm_xor_si128((dataL)[0], (tmpsL)[0]);

		_mm_storeu_si128((__m128i *)padded, tmpsL[0]);
		memcpy(C + Mlen - rem, padded, rem);
		memcpy(hashblk, C + Mlen - rem, rem);
		hashblk[rem] = 0x01;

		X = _mm_xor_si128(X, _mm_loadu_si128((__m128i *)hashblk));
		return polydot128(ctx->poly, X, ctx->htbl[0]);
	}
}

static inline __m128i middlelowerx8(aespolyW_context* ctx, __m128i state, __m128i S1, __m128i S2, const uint8_t *M, size_t Mlen, uint8_t *C)
{
	size_t m = (Mlen + 15) / 16;
	size_t rem = Mlen + 16 - m * 16;
	size_t dlen = MAX(m - 1, 0) / 2;
	size_t dx8rem = dlen % 8;
	size_t dx8len = dlen / 8;

	alignas(16) __m128i data[16];
	alignas(16) __m128i tmps[24];
	alignas(16) __m128i omegai;

	__m128i *dataL = data;
	__m128i *dataR = data + 8;

	__m128i *tmpsL = tmps;
	__m128i *tmpsR = tmps + 8;
	__m128i *mask = tmps + 16;

	setzero_x4(mask);

	alignas(16) __m128i ctr[8] = {
		_mm_setr_epi32(1, 0, 0, 0),
		_mm_setr_epi32(2, 0, 0, 0),
		_mm_setr_epi32(3, 0, 0, 0),
		_mm_setr_epi32(4, 0, 0, 0),
		_mm_setr_epi32(5, 0, 0, 0),
		_mm_setr_epi32(6, 0, 0, 0),
		_mm_setr_epi32(7, 0, 0, 0),
		_mm_setr_epi32(8, 0, 0, 0)
	};
	alignas(16) __m128i inc4 = _mm_setr_epi32(4, 0, 0, 0);
	alignas(16) __m128i inc = _mm_setr_epi32(1, 0, 0, 0);

	alignas(16) __m128i X = state;
	alignas(16) __m128i Y = _mm_setzero_si128(), Z = _mm_setzero_si128();

	omegai = ctx->omega[0];

	for (size_t i = 0; i < dx8len; i++)
	{
		gather_load_x8((__m128i *)M + 16 * i, dataL);
		gather_load_x8((__m128i *)M + 16 * i + 1, dataR);

		seq_graycode_x8(mask, ctx->omega, ctx->omega[2], omegai, i);

		Y = polyreduce128(ctx->poly, Y);
		Z = _mm_xor_si128(X, Y);

		middle_round(dataR, dataL, tmpsR, tmpsL, ctx->aes_ctx.keys128, ctr, S1, S2, 0);
		middle_round(dataR, dataL, tmpsR, tmpsL, ctx->aes_ctx.keys128, ctr, S1, S2, 1);
		middle_round(dataR, dataL, tmpsR, tmpsL, ctx->aes_ctx.keys128, ctr, S1, S2, 2);
		middle_round(dataR, dataL, tmpsR, tmpsL, ctx->aes_ctx.keys128, ctr, S1, S2, 3);
		middle_round(dataR, dataL, tmpsR, tmpsL, ctx->aes_ctx.keys128, ctr, S1, S2, 4);
		middle_round(dataR, dataL, tmpsR, tmpsL, ctx->aes_ctx.keys128, ctr, S1, S2, 5);
		middle_round(dataR, dataL, tmpsR, tmpsL, ctx->aes_ctx.keys128, ctr, S1, S2, 6);
		middle_round(dataR, dataL, tmpsR, tmpsL, ctx->aes_ctx.keys128, ctr, S1, S2, 7);

		copyx8(tmpsL, dataL);

		Z = _mm_xor_si128(Z, dataL[0]);

		tmpsR[0] = aesenc128(tmpsR[0], ctx->aes_ctx.keys128);
		tmpsR[0] = _mm_xor_si128(tmpsR[0], mask[0]);
		schoolbook_initialadd128(dataL[7], ctx->htbl[0], tmpsL);

		lower_round(tmpsR, dataL, tmpsR, tmpsL, ctx->aes_ctx.keys128, mask, ctx->htbl, 8, 1);
		lower_round(tmpsR, dataL, tmpsR, tmpsL, ctx->aes_ctx.keys128, mask, ctx->htbl, 8, 2);
		lower_round(tmpsR, dataL, tmpsR, tmpsL, ctx->aes_ctx.keys128, mask, ctx->htbl, 8, 3);
		lower_round(tmpsR, dataL, tmpsR, tmpsL, ctx->aes_ctx.keys128, mask, ctx->htbl, 8, 4);
		lower_round(tmpsR, dataL, tmpsR, tmpsL, ctx->aes_ctx.keys128, mask, ctx->htbl, 8, 5);
		lower_round(tmpsR, dataL, tmpsR, tmpsL, ctx->aes_ctx.keys128, mask, ctx->htbl, 8, 6);

		tmpsR[7] = aesenc128(tmpsR[7], ctx->aes_ctx.keys128);
		tmpsR[7] = _mm_xor_si128(tmpsR[7], mask[7]);
		schoolbook_add128(Z, ctx->htbl[7], tmpsL);

		scatter_store_x8((__m128i *)C + 16 * i, dataL);
		scatter_store_x8((__m128i *)C + 16 * i + 1, tmpsR);

		tmpsL[3] = _mm_bsrli_si128(tmpsL[2], 8);
		tmpsL[2] = _mm_bslli_si128(tmpsL[2], 8);

		X = _mm_xor_si128(tmpsL[3], tmpsL[1]);
		Y = _mm_xor_si128(tmpsL[0], tmpsL[2]);

		addx8_bfix(ctr, inc4, ctr);
	}
	Y = polyreduce128(ctx->poly, Y);
	Z = _mm_xor_si128(X, Y);
	X = Z;

	for (size_t i = 0; i < dx8rem; i++)
	{
		gather_load_x1((__m128i *)M + dx8len * 8 + 2 * i, dataL);
		gather_load_x1((__m128i *)M + dx8len * 8 + 2 * i + 1, dataR);

		middle_round(dataR, dataL, tmpsR, tmpsL, ctx->aes_ctx.keys128, ctr, S1, S2, 0);

		seq_graycode_x1(mask, ctx->omega, dx8len * 4 + i);

		tmpsR[0] = aesenc128(tmpsR[0], ctx->aes_ctx.keys128);
		tmpsR[0] = _mm_xor_si128(tmpsR[0], mask[0]);

		scatter_store_x1((__m128i *)C + dx8len * 8 + 2 * i, tmpsL);
		scatter_store_x1((__m128i *)C + dx8len * 8 + 2 * i + 1, tmpsR);

		X = _mm_xor_si128(X, tmpsL[0]);
		X = polydot128(ctx->poly, X, ctx->htbl[0]);
		ctr[0] = _mm_add_epi64(ctr[0], inc);
	}

	if (m % 2 == 0)
	{
		dataR[0] = _mm_loadu_si128((__m128i *)(M + Mlen - 16));

		uint8_t padded[17];
		uint8_t hashblk[17];
		memset(padded, 0, 16);
		memset(hashblk, 0, 16);
		memcpy(padded, M + Mlen - 16 - rem, rem);
		dataL[0] = _mm_loadu_si128((__m128i *)padded);

		middle_round(dataR, dataL, tmpsR, tmpsL, ctx->aes_ctx.keys128, ctr, S1, S2, 0);
		seq_graycode_x1(mask, ctx->omega, dlen);

		tmpsR[0] = aesenc128(tmpsR[0], ctx->aes_ctx.keys128);
		tmpsR[0] = _mm_xor_si128(tmpsR[0], mask[0]);

		_mm_storeu_si128((__m128i *)(C + Mlen - 16), tmpsR[0]);

		_mm_storeu_si128((__m128i *)padded, tmpsL[0]);
		memcpy(C + Mlen - rem, padded, rem);
		memcpy(hashblk, C + Mlen - rem, rem);
		hashblk[rem] = 0x01;

		X = _mm_xor_si128(X, _mm_loadu_si128((__m128i *)hashblk));
		return polydot128(ctx->poly, X, ctx->htbl[0]);
	}
	else
	{
		uint8_t padded[17];
		uint8_t hashblk[17];
		memset(padded, 0, 16);
		memset(hashblk, 0, 16);
		memcpy(padded, M + Mlen - rem, rem);
		dataL[0] = _mm_loadu_si128((__m128i *)padded);

		(tmpsL)[0] = _mm_xor_si128((ctr)[0], S1);
		(tmpsL)[0] = aesenc128((tmpsL)[0], ctx->aes_ctx.keys128);
		(tmpsL)[0] = _mm_xor_si128((dataL)[0], (tmpsL)[0]);

		_mm_storeu_si128((__m128i *)padded, tmpsL[0]);
		memcpy(C + Mlen - rem, padded, rem);
		memcpy(hashblk, C + Mlen - rem, rem);
		hashblk[rem] = 0x01;

		X = _mm_xor_si128(X, _mm_loadu_si128((__m128i *)hashblk));
		return polydot128(ctx->poly, X, ctx->htbl[0]);
	}
}

static inline void init(aespolyW_context *ctx, uint8_t *key)
{
	aesinit128(&(ctx->aes_ctx), key);

	ctx->poly_double = _mm_setr_epi32(0x87, 0, 0, 0);
	ctx->poly = _mm_setr_epi32(0x1, 0, 0, 0xc2000000);
	__m128i pp = _mm_slli_si128(ctx->poly_double, 1);
	ctx->L[0] = aesenc128(_mm_setzero_si128(), ctx->aes_ctx.keys128);
	ctx->omega[0] = aesenc128(_mm_setr_epi32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff), ctx->aes_ctx.keys128);
	init_htbl128(ctx->htbl, ctx->poly, ctx->L[0]);

	for (size_t i = 1; i < 250; i++)
	{
		ctx->L[i] = mul2rev_xex(pp, ctx->L[i - 1]);
		ctx->omega[i] = mul2rev_xex(pp, ctx->omega[i - 1]);
	}
}

static inline void init_with_h(aespolyW_context *ctx, uint8_t *key, uint8_t *h)
{
	init(ctx, key);
	init_htbl128(ctx->htbl, ctx->poly, _mm_loadu_si128((__m128i *)h));
}

static inline void encx4(aespolyW_context ctx, uint8_t *T, size_t Tlen, uint8_t *M, size_t Mlen, uint8_t *C)
{
	uint8_t *MN = M;
	uint8_t *MLR = M + 16;
	uint8_t *CN = C;
	uint8_t *CLR = C + 16;

	size_t MLRlen = MAX(Mlen - 16, 0);

	alignas(16) __m128i tmps[8];

	bool mln = MLRlen % 16 > 0;

	size_t tr_blocks = ((Tlen + 15) / 16) / 2;
	size_t Tr_len = tr_blocks * 16;
	size_t Tl_len = Tlen - Tr_len;
	tmps[0] = tweaker_scalar(ctx, mln, T, Tl_len);
	tmps[1] = phash(ctx, T + Tl_len, Tr_len);

	tmps[2] = _mm_loadu_si128((__m128i *)MN);
	tmps[2] = _mm_xor_si128(tmps[1], tmps[2]);

	upper(ctx, tmps[0], MLR, MLRlen, CLR, tmps + 3, tmps + 4);

	tmps[2] = _mm_xor_si128(tmps[2], tmps[3]);
	tmps[2] = _mm_xor_si128(tmps[2], tmps[4]);

	tmps[2] = aesenc128(tmps[2], ctx.aes_ctx.keys128);
	tmps[6] = tmps[2];

	tmps[5] = middlelower(&ctx, tmps[0], tmps[2], tmps[2], MLR, MLRlen, CLR);

	tmps[2] = aesenc128(tmps[2], ctx.aes_ctx.keys128);

	tmps[2] = _mm_xor_si128(tmps[5], tmps[2]);
	tmps[2] = _mm_xor_si128(tmps[1], tmps[2]);

	if ((MLRlen / 16) % 4 > 0)
	{
		tmps[2] = _mm_xor_si128(tmps[6], tmps[2]);
	}

	_mm_storeu_si128((__m128i *)C, tmps[2]);
}

static inline void encx8(aespolyW_context ctx, uint8_t *T, size_t Tlen, uint8_t *M, size_t Mlen, uint8_t *C)
{
	uint8_t *MN = M;
	uint8_t *MLR = M + 16;
	uint8_t *CN = C;
	uint8_t *CLR = C + 16;

	size_t MLRlen = MAX(Mlen - 16, 0);

	alignas(16) __m128i tmps[8];

	bool mln = MLRlen % 16 > 0;

	size_t tr_blocks = ((Tlen + 15) / 16) / 2;
	size_t Tr_len = tr_blocks * 16;
	size_t Tl_len = Tlen - Tr_len;
	tmps[0] = tweaker_scalar(ctx, mln, T, Tl_len);
	tmps[1] = phash(ctx, T + Tl_len, Tr_len);

	tmps[2] = _mm_loadu_si128((__m128i *)MN);
	tmps[2] = _mm_xor_si128(tmps[1], tmps[2]);

	upperx8(ctx, tmps[0], MLR, MLRlen, CLR, tmps + 3, tmps + 4);

	tmps[2] = _mm_xor_si128(tmps[2], tmps[3]);
	tmps[2] = _mm_xor_si128(tmps[2], tmps[4]);

	tmps[2] = aesenc128(tmps[2], ctx.aes_ctx.keys128);
	tmps[6] = tmps[2];

	tmps[5] = middlelowerx8(&ctx, tmps[0], tmps[2], tmps[2], MLR, MLRlen, CLR);

	tmps[2] = aesenc128(tmps[2], ctx.aes_ctx.keys128);

	tmps[2] = _mm_xor_si128(tmps[5], tmps[2]);
	tmps[2] = _mm_xor_si128(tmps[1], tmps[2]);

	if ((MLRlen / 16) % 4 > 0)
	{
		tmps[2] = _mm_xor_si128(tmps[6], tmps[2]);
	}

	_mm_storeu_si128((__m128i *)C, tmps[2]);
}
