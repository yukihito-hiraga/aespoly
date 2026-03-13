#pragma once
#include "core.h"

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

void init_htbl128(__m128i *htbl, __m128i poly, __m128i H)
{
	htbl[0] = _mm_setr_epi32(0x1, 0, 0, 0xc2000000);
	for (size_t i = 1; i < 5; i++)
	{
		htbl[i] = polydot128(poly, htbl[i - 1], H);
	}
}

void ghash_polyinit128(aes_context *aesctx, ghash_poly_context *ctx, uint8_t *key)
{
	ctx->poly = _mm_setr_epi32(0x1, 0, 0, 0xc2000000);

	aesinit128(aesctx, key);
	alignas(16) __m128i H = aesenc128(_mm_setzero_si128(), aesctx->keys128);
	init_htbl128(ctx->htbl, ctx->poly, H);
}

#define encxor(dataL, dataR, tmpsL, tmpsR, keys, i)   \
	{                                                 \
		tmpsR[i] = aesenc128(dataR[i], keys);         \
		tmpsL[i] = _mm_xor_si128(tmpsR[i], dataL[i]); \
	}

static inline __m128i ghash_poly128(aes_context aesctx, ghash_poly_context ctx, const uint8_t *A, size_t Alen, const uint8_t *M, size_t Mlen)
{
	size_t blen = (Alen + Mlen) / 16;
	size_t dlen = blen / 2;
	size_t dlenx4 = dlen / 4;
	size_t dremx4 = dlen % 4;

	alignas(16) __m128i data[8];
	alignas(16) __m128i tmps[12];

	__m128i *dataL = data;
	__m128i *dataR = data + 4;

	__m128i *tmpsL = tmps;
	__m128i *tmpsR = tmps + 4;
	__m128i *tmpsH = tmps + 8;

	alignas(16) __m128i X = _mm_setzero_si128();
	alignas(16) __m128i Y = _mm_setzero_si128(), Z = _mm_setzero_si128();

	alignas(16) __m128i S = _mm_setzero_si128();

	for (size_t i = 0; i < dlenx4; i++)
	{
		gather_load_x4((__m128i *)M + 8 * i, dataL);
		gather_load_x4((__m128i *)M + 8 * i + 1, dataR);

		Y = polyreduce128(ctx.poly, Y);
		Z = _mm_xor_si128(X, Y);

		encxor(dataL, dataR, tmpsL, tmpsR, aesctx.keys128, 0);
		encxor(dataL, dataR, tmpsL, tmpsR, aesctx.keys128, 3);
		encxor(dataL, dataR, tmpsL, tmpsR, aesctx.keys128, 2);
		encxor(dataL, dataR, tmpsL, tmpsR, aesctx.keys128, 1);
		Z = _mm_xor_si128(Z, tmpsL[0]);

		schoolbook_initialadd128(tmpsL[3], ctx.htbl[0], tmpsH);
		schoolbook_add128(tmpsL[2], ctx.htbl[1], tmpsH);
		schoolbook_add128(tmpsL[1], ctx.htbl[2], tmpsH);

		schoolbook_add128(Z, ctx.htbl[3], tmpsH);

		tmpsH[3] = _mm_bsrli_si128(tmpsH[2], 8);
		tmpsH[2] = _mm_bslli_si128(tmpsH[2], 8);

		X = _mm_xor_si128(tmpsH[3], tmpsH[1]);
		Y = _mm_xor_si128(tmpsH[0], tmpsH[2]);
	}

	Y = polyreduce128(ctx.poly, Y);
	Z = _mm_xor_si128(X, Y);
	X = Z;

	for (size_t i = 0; i < dlen % 4; i++)
	{
		data[0] = _mm_loadu_si128((__m128i *)M + dlenx4 * 8 + 2 * i);
		data[1] = _mm_loadu_si128((__m128i *)M + dlenx4 * 8 + 2 * i + 1);

		tmps[0] = aesenc128(data[1], aesctx.keys128);
		tmps[0] = _mm_xor_si128(tmps[0], data[0]);

		X = _mm_xor_si128(X, tmps[0]);
		X = polydot128(ctx.poly, X, ctx.htbl[0]);
	}

	if (blen % 2 > 0)
	{
	}

	X = aesenc128(X, aesctx.keys128);

	X = _mm_xor_si128(X, _mm_setr_epi64(_m_from_int64(Alen), _m_from_int64(Mlen)));

	X = aesenc128(X, aesctx.keys128);

	return X;
}

#define ecbpoly_round(n, i)                                            \
	{                                                                  \
		encxor(dataL, dataR, tmpsL, tmpsR, aesctx.keys128, n - 1 - i); \
		schoolbook_add128(tmpsL[n - 1 - i], ctx.htbl[i], tmpsH);       \
	}

#define encxorp(dataL, dataR, tmpsL, tmpsR, tmpsP, keys, i) \
	{                                                       \
		tmpsR[i] = aesenc128(dataR[i], keys);               \
		tmpsL[i] = _mm_xor_si128(tmpsP[i], dataL[i]);       \
	}

static inline __m128i ghash_poly128xx(aes_context aesctx, ghash_poly_context ctx, const uint8_t *A, size_t Alen, const uint8_t *M, size_t Mlen)
{
	const size_t xx = 8;

	size_t blen = (Alen + Mlen) / 16;
	size_t dlen = blen / 2;
	size_t dlenxx = dlen / xx;
	size_t dremxx = dlen % xx;

	alignas(16) __m128i data[20];
	alignas(16) __m128i tmps[40];

	__m128i *dataL = data;
	__m128i *dataR = data + xx;

	__m128i *tmpsL = tmps;
	__m128i *tmpsR = tmps + xx;
	__m128i *tmpsH = tmps + xx * 2;
	__m128i *tmpsP = tmps + xx * 3;

	alignas(16) __m128i X = _mm_setzero_si128();
	alignas(16) __m128i Y = _mm_setzero_si128(), Z = _mm_setzero_si128();

	// gather_load_x8((__m128i *)M + xx * 2 * 0 + 1, dataR);
	// aesx8(aesctx.keys128, dataR, tmpsP);

	for (size_t i = 0; i < dlenxx; i++)
	{
		gather_load_x8((__m128i *)M + xx * 2 * i, dataL);
		gather_load_x8((__m128i *)M + xx * 2 * (i + 0) + 1, dataR);

		Y = polyreduce128(ctx.poly, Y);
		Z = _mm_xor_si128(X, Y);

		aesx4(aesctx.keys128, dataR + 4, tmpsP + 4);
		xorx4_1wise(dataL + 4, tmpsP + 4, tmpsL + 4);

		schoolbook_initialadd128(tmpsL[7], ctx.htbl[0], tmpsH);
		schoolbook_add128(tmpsL[6], ctx.htbl[1], tmpsH);
		schoolbook_add128(tmpsL[5], ctx.htbl[2], tmpsH);
		schoolbook_add128(tmpsL[4], ctx.htbl[3], tmpsH);

		aesx4(aesctx.keys128, dataR + 0, tmpsP + 0);
		xorx4_1wise(dataL + 0, tmpsP + 0, tmpsL + 0);
		Z = _mm_xor_si128(Z, tmpsL[0]);

		schoolbook_add128(tmpsL[3], ctx.htbl[4], tmpsH);
		schoolbook_add128(tmpsL[2], ctx.htbl[5], tmpsH);
		schoolbook_add128(tmpsL[1], ctx.htbl[6], tmpsH);
		schoolbook_add128(Z, ctx.htbl[7], tmpsH);

		tmpsH[3] = _mm_bsrli_si128(tmpsH[2], 8);
		tmpsH[2] = _mm_bslli_si128(tmpsH[2], 8);

		X = _mm_xor_si128(tmpsH[3], tmpsH[1]);
		Y = _mm_xor_si128(tmpsH[0], tmpsH[2]);
	}

	Y = polyreduce128(ctx.poly, Y);
	Z = _mm_xor_si128(X, Y);
	X = Z;

	for (size_t i = 0; i < dremxx; i++)
	{
		data[0] = _mm_loadu_si128((__m128i *)M + dlenxx * xx * 2 + 2 * i);
		data[1] = _mm_loadu_si128((__m128i *)M + dlenxx * xx * 2 + 2 * i + 1);

		tmps[0] = aesenc128(data[1], aesctx.keys128);
		tmps[0] = _mm_xor_si128(tmps[0], data[0]);

		X = _mm_xor_si128(X, tmps[0]);
		X = polydot128(ctx.poly, X, ctx.htbl[0]);
	}

	if (blen % 2 > 0)
	{
	}

	X = aesenc128(X, aesctx.keys128);

	X = _mm_xor_si128(X, _mm_setr_epi64(_m_from_int64(Alen), _m_from_int64(Mlen)));

	X = aesenc128(X, aesctx.keys128);

	return X;
}
