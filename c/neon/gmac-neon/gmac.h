#pragma once
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

static inline __m128i fmul128(__m128i poly, __m128i a, __m128i b)
{
	alignas(16) __m128i tmp0, tmp1, tmp2, tmp3, X, Y;
	tmp0 = _mm_clmulepi64_si128(a, b, 0x00);
	tmp1 = _mm_clmulepi64_si128(a, b, 0x11);
	tmp2 = _mm_clmulepi64_si128(a, b, 0x10);
	tmp3 = _mm_clmulepi64_si128(a, b, 0x01);
	tmp2 = _mm_xor_si128(tmp2, tmp3);

	X = _mm_xor_si128(tmp0, _mm_slli_si128(tmp2, 8));
	Y = _mm_xor_si128(tmp1, _mm_srli_si128(tmp2, 8));

	tmp0 = _mm_clmulepi64_si128(poly, Y, 0x00);
	tmp1 = _mm_clmulepi64_si128(poly, Y, 0x11);
	tmp2 = _mm_clmulepi64_si128(poly, Y, 0x10);
	tmp3 = _mm_clmulepi64_si128(poly, Y, 0x01);
	tmp2 = _mm_xor_si128(tmp2, tmp3);

	X = _mm_xor_si128(X, _mm_xor_si128(tmp0, _mm_slli_si128(tmp2, 8)));
	Y = _mm_xor_si128(tmp1, _mm_srli_si128(tmp2, 8));

	X = _mm_xor_si128(X, _mm_clmulepi64_si128(poly, Y, 0x00));

	return X;
}

void init_htbl128(__m128i *htbl, __m128i poly, __m128i H)
{
	htbl[0] = polydot128(poly, poly, H);
	for (size_t i = 1; i < 8; i++)
	{
		htbl[i] = polydot128(poly, htbl[i - 1], H);
	}
}

void gmacinit128(aes_context *aesctx, gmac_context *ctx, uint8_t *key)
{
	ctx->poly = _mm_setr_epi32(0x1, 0, 0, 0xc2000000);

	aesinit128(aesctx, key);
	alignas(16) __m128i H = aesenc128(_mm_setzero_si128(), aesctx->keys128);
	H = byterev(H);
	H = fmul128(ctx->poly, H, _mm_setr_epi32(2, 0, 0, 0));

	init_htbl128(ctx->htbl, ctx->poly, H);
}

static inline __m128i polyval128(gmac_context ctx, __m128i state, __m128i *blks, size_t blen)
{
	alignas(16) __m128i H0 = ctx.htbl[1];
	alignas(16) __m128i X = state;
	alignas(16) __m128i data, tmp0, tmp1;
	size_t loopnum = blen / 16;
	for (size_t i = 0; i < loopnum; i++)
	{
		data = _mm_loadu_si128(blks + i);
		X = _mm_xor_si128(X, data);
		X = polydot128(ctx.poly, X, H0);
	}

	return X;
}

static inline __m128i polyval128x4(gmac_context ctx, __m128i state, __m128i *blks, size_t blen)
{

	alignas(16) __m128i tmps[4];
	alignas(16) __m128i data[4];

	alignas(16) __m128i X = state;
	alignas(16) __m128i Y = _mm_setzero_si128(), Z = _mm_setzero_si128();

	size_t loopnum = blen / 16;
	size_t remainder = loopnum % 4;
	for (size_t i = 0; i < (loopnum / 4); i++)
	{
		loadx4(blks + i * 4, data);

		data[0] = byterev(data[0]);
		data[1] = byterev(data[1]);
		data[2] = byterev(data[2]);
		data[3] = byterev(data[3]);

		Y = polyreduce128(ctx.poly, Y);
		Z = _mm_xor_si128(X, Y);
		data[0] = _mm_xor_si128(Z, data[0]);

		schoolbook_initialadd128(data[3], ctx.htbl[0], tmps);

		schoolbook_add128(data[2], ctx.htbl[1], tmps);
		schoolbook_add128(data[1], ctx.htbl[2], tmps);
		schoolbook_add128(data[0], ctx.htbl[3], tmps);

		tmps[3] = _mm_bsrli_si128(tmps[2], 8);
		tmps[2] = _mm_bslli_si128(tmps[2], 8);

		X = _mm_xor_si128(tmps[3], tmps[1]);
		Y = _mm_xor_si128(tmps[0], tmps[2]);
	}
	Y = polyreduce128(ctx.poly, Y);
	Z = _mm_xor_si128(X, Y);
	X = Z;
	for (size_t i = 0; i < remainder; i++)
	{
		data[0] = _mm_loadu_si128(blks + loopnum - remainder + i);
		X = _mm_xor_si128(X, byterev(data[0]));
		X = polydot128(ctx.poly, X, ctx.htbl[0]);
	}
	return X;
}

static inline __m128i polyval128x8(gmac_context ctx, __m128i state, __m128i *P, size_t Plen)
{

	alignas(16) __m128i tmps[4];

	alignas(16) __m128i X = state;
	alignas(16) __m128i Y = _mm_setzero_si128(), Z = _mm_setzero_si128();
	alignas(16) __m128i data[8];

	size_t loopnum = Plen / 16;
	size_t remainder = loopnum % 8;
	for (size_t i = 0; i < (loopnum / 8); i++)
	{
		loadx8((__m128i *)P + i * 8, data);

		data[0] = byterev(data[0]);
		data[1] = byterev(data[1]);
		data[2] = byterev(data[2]);
		data[3] = byterev(data[3]);
		data[4] = byterev(data[4]);
		data[5] = byterev(data[5]);
		data[6] = byterev(data[6]);
		data[7] = byterev(data[7]);

		Y = polyreduce128(ctx.poly, Y);
		Z = _mm_xor_si128(X, Y);
		Z = _mm_xor_si128(Z, data[0]);

		schoolbook_initialadd128(data[7], ctx.htbl[0], tmps);
		schoolbook_add128(data[6], ctx.htbl[1], tmps);
		schoolbook_add128(data[5], ctx.htbl[2], tmps);
		schoolbook_add128(data[4], ctx.htbl[3], tmps);
		schoolbook_add128(data[3], ctx.htbl[4], tmps);
		schoolbook_add128(data[2], ctx.htbl[5], tmps);
		schoolbook_add128(data[1], ctx.htbl[6], tmps);
		schoolbook_add128(Z, ctx.htbl[7], tmps);

		tmps[3] = _mm_bsrli_si128(tmps[2], 8);
		tmps[2] = _mm_bslli_si128(tmps[2], 8);

		X = _mm_xor_si128(tmps[3], tmps[1]);
		Y = _mm_xor_si128(tmps[0], tmps[2]);
	}
	Y = polyreduce128(ctx.poly, Y);
	Z = _mm_xor_si128(X, Y);
	X = Z;
	for (size_t i = 0; i < remainder; i++)
	{
		data[0] = _mm_loadu_si128((__m128i *)P + loopnum - remainder + i);
		X = _mm_xor_si128(X, byterev(data[0]));
		X = polydot128(ctx.poly, X, ctx.htbl[0]);
	}
	return X;
}

static inline __m128i ghash128x4(gmac_context ctx, uint8_t *A, size_t Alen, uint8_t* N, size_t Nlen)
{
	alignas(16) __m128i X = _mm_setzero_si128();
	size_t rem = Alen % 16;
	X = polyval128x4(ctx, X, (__m128i *)A, Alen - rem);

	uint8_t padded[16];
	alignas(16) __m128i paddedblk = _mm_setzero_si128();
	if (rem > 0)
	{
		memset(padded, 0, 16);
		memcpy(padded, A + Alen - rem, rem);
		paddedblk = _mm_loadu_si128(((__m128i *)padded));

		X = _mm_xor_si128(X, byterev(paddedblk));
		X = polydot128(ctx.poly, X, ctx.htbl[0]);
	}
	alignas(16) uint8_t chunk[16];
	memset(chunk, 0, 16);
	((uint64_t *)(chunk))[1] = Alen * 8;
	alignas(16) __m128i blk = _mm_loadu_si128((__m128i *)chunk);
	X = _mm_xor_si128(X, blk);
	X = polydot128(ctx.poly, X, ctx.htbl[0]);

	return byterev(X);
}

static inline __m128i ghash128x8(gmac_context ctx, uint8_t *A, size_t Alen, uint8_t* N, size_t Nlen)
{
	alignas(16) __m128i X = _mm_setzero_si128();
	size_t rem = Alen % 16;
	X = polyval128x8(ctx, X, (__m128i *)A, Alen - rem);

	uint8_t padded[16];
	alignas(16) __m128i paddedblk = _mm_setzero_si128();
	if (rem > 0)
	{
		memset(padded, 0, 16);
		memcpy(padded, A + Alen - rem, rem);
		paddedblk = _mm_loadu_si128(((__m128i *)padded));

		X = _mm_xor_si128(X, byterev(paddedblk));
		X = polydot128(ctx.poly, X, ctx.htbl[0]);
	}
	alignas(16) uint8_t chunk[16];
	memset(chunk, 0, 16);
	((uint64_t *)(chunk))[1] = Alen * 8;
	alignas(16) __m128i blk = _mm_loadu_si128((__m128i *)chunk);
	X = _mm_xor_si128(X, blk);
	X = polydot128(ctx.poly, X, ctx.htbl[0]);

	return byterev(X);
}
