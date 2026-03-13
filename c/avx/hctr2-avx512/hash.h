#pragma once
#include "../common/common.h"

#include "core.h"

inline __m512i polyreduce512(hctr2_context* ctx, __m512i x)
{
	alignas(512) __m512i x0 = _mm512_clmulepi64_epi128(x, ctx->poly, 0x10);
	alignas(512) __m512i y0 = _mm512_shuffle_epi32(x, 78);
	alignas(512) __m512i y1 = _mm512_xor_si512(y0, x0);
	alignas(512) __m512i x1 = _mm512_clmulepi64_epi128(y1, ctx->poly, 0x10);
	alignas(512) __m512i y2 = _mm512_shuffle_epi32(y1, 78);
	return _mm512_xor_si512(y2, x1);
}

inline __m512i polydot512(hctr2_context* ctx, __m512i a, __m512i b)
{
	alignas(512) __m512i pp00 = _mm512_clmulepi64_epi128(a, b, 0x00);
	alignas(512) __m512i pp11 = _mm512_clmulepi64_epi128(a, b, 0x11);
	alignas(512) __m512i pp10 = _mm512_clmulepi64_epi128(a, b, 0x01);
	alignas(512) __m512i pp01 = _mm512_clmulepi64_epi128(a, b, 0x10);

	alignas(512) __m512i ppmid = _mm512_xor_si512(pp10, pp01);
	alignas(512) __m512i ppmid_ls = _mm512_bslli_epi128(ppmid, 8);
	alignas(512) __m512i ppmid_rs = _mm512_bsrli_epi128(ppmid, 8);
	alignas(512) __m512i ppupper = _mm512_xor_si512(ppmid_rs, pp11);
	alignas(512) __m512i pplower = _mm512_xor_si512(ppmid_ls, pp00);
	alignas(512) __m512i ppl_reduced = polyreduce512(ctx, pplower);
	return _mm512_xor_si512(ppupper, ppl_reduced);
}

// required blen to be divisable by blocklength
inline __m512i polyval512(hctr2_context* ctx, __m512i state, uint8_t *blks, size_t blen)
{
	alignas(512) __m512i H0 = ctx->htbl[0];
	alignas(512) __m512i Hx4 = ctx->hx4tbl[0];
	alignas(512) __m512i HH = ctx->hh;
	alignas(512) __m512i X0 = state;
	alignas(512) __m512i X = _mm512_setzero_si512();
	alignas(512) __m512i Y = X0;
	alignas(512) __m512i Z = _mm512_setzero_si512();
	alignas(512) __m512i data, tmp0, tmp1;
	size_t loopnum = blen / 16;
	size_t remainder = loopnum % 4;
	alignas(128) __m128i tmp = ((__m128i *)&X0)[0];
	X0 = _mm512_setzero_si512();
	((__m128i *)&X0)[0] = tmp;
	for (size_t i = 0; i < (loopnum / 4); i++)
	{

		data = _mm512_loadu_si512((__m512i*)blks + (i * 4));
		X = Y;
		X = _mm512_xor_si512(X, data);
		Y = polydot512(ctx, X, Hx4);
	}
	if (loopnum >= 4)
	{
		X0 = polydot512(ctx, X, H0);
	}

	((__m128i *)&tmp0)[0] = ((__m128i *)&X0)[2];
	((__m128i *)&tmp0)[1] = ((__m128i *)&X0)[3];
	tmp0 = _mm512_xor_si512(X0, tmp0);
	X = _mm512_setzero_si512();
	((__m128i *)&X)[0] = _mm_xor_si128(((__m128i *)&tmp0)[0], ((__m128i *)&tmp0)[1]);

	for (size_t i = 0; i < remainder; i++)
	{
		data = _mm512_setzero_si512();
		((__m128i *)&data)[3] = ((__m128i*)blks)[loopnum - remainder + i];
		X = _mm512_xor_si512(X, data);
		X = polydot512(ctx, X, HH);
	}
	((__m128i *)&data)[0] = ((__m128i *)&X)[2];
	((__m128i *)&data)[1] = ((__m128i *)&X)[3];
	X = _mm512_xor_si512(X, data);
	tmp = _mm_xor_si128(((__m128i *)&X)[0], ((__m128i *)&X)[1]);
	X = _mm512_setzero_si512();
	((__m128i *)&X)[0] = tmp;
	return X;
}

static inline __m512i polyval512x4(hctr2_context* ctx, __m512i state, uint8_t *P, size_t Plen)
{

	alignas(512) __m512i tmps[4];

	alignas(512) __m512i X = state;
	alignas(512) __m512i Y = _mm512_setzero_si512(), Z = _mm512_setzero_si512();
	alignas(512) __m512i data[4];

	size_t blen = Plen / 16;
	size_t qlen = blen / 4;
	size_t qrem = blen % 4;
	size_t qx4len = qlen / 4;
	size_t qx4rem = qlen % 4;

	alignas(128) __m128i tmp = ((__m128i *)&X)[0];
	X = _mm512_setzero_si512();

	((__m128i *)&X)[0] = tmp;

	for (size_t i = 0; i < qx4len; i++)
	{
		data[0] = _mm512_loadu_si512((__m512*)P + (i * 4 + 0));
		data[1] = _mm512_loadu_si512((__m512*)P + (i * 4 + 1));
		data[2] = _mm512_loadu_si512((__m512*)P + (i * 4 + 2));
		data[3] = _mm512_loadu_si512((__m512*)P + (i * 4 + 3));

		Y = polyreduce512(ctx, Y);
		Z = _mm512_xor_si512(X, Y);

		Z = _mm512_xor_si512(Z, data[0]);

		schoolbook_initialadd512(tmps, data[3], ctx->htbl[0]);

		schoolbook_add512(tmps, data[2], ctx->htbl[1]);
		schoolbook_add512(tmps, data[1], ctx->htbl[2]);
		schoolbook_add512(tmps, Z, ctx->htbl[3]);

		tmps[3] = _mm512_bsrli_epi128(tmps[2], 8);
		tmps[2] = _mm512_bslli_epi128(tmps[2], 8);

		X = _mm512_xor_si512(tmps[3], tmps[1]);
		Y = _mm512_xor_si512(tmps[0], tmps[2]);
	}
	Y = polyreduce512(ctx, Y);
	Z = _mm512_xor_si512(X, Y);
	((__m128i *)&tmps)[0] = ((__m128i *)&Z)[2];
	((__m128i *)&tmps)[1] = ((__m128i *)&Z)[3];
	tmps[0] = _mm512_xor_si512(Z, tmps[0]);
	X = _mm512_setzero_si512();
	((__m128i *)&X)[0] = _mm_xor_si128(((__m128i *)&tmps)[0], ((__m128i *)&tmps)[1]);

	for (size_t i = 0; i < qx4rem*4+qrem; i++)
	{
		data[0] = _mm512_setzero_si512();
		((__m128i *)&data)[3] = ((__m128i*)P)[16*qx4len + i];
		X = _mm512_xor_si512(X, data[0]);
		X = polydot512(ctx, X, ctx->htbl[0]);
	}
	((__m128i *)&data)[0] = ((__m128i *)&X)[2];
	((__m128i *)&data)[1] = ((__m128i *)&X)[3];
	X = _mm512_xor_si512(X, data[0]);
	tmp = _mm_xor_si128(((__m128i *)&X)[0], ((__m128i *)&X)[1]);
	X = _mm512_setzero_si512();
	((__m128i *)&X)[0] = tmp;
	return X;
}

static inline __m512i hash512(hctr2_context* ctx, uint8_t *M, size_t mlen, uint8_t *T, size_t tlen)
{
	alignas(64) __m512i HH = ctx->hh;
	size_t len = 2 * 8 * tlen + 2;
	if (mlen % 16 != 0)
	{
		len += 1;
	}
	alignas(512) __m512i firstblk = CAST2M512i(len);
	alignas(512) __m512i X = polydot512(ctx, HH, firstblk);
	size_t remainder = tlen % 16;
	if (tlen >= 16)
	{
		X = polyval512(ctx, X, T, tlen - remainder);
	}
	uint8_t padded[16];
	alignas(512) __m512i paddedblk = _mm512_setzero_si512();
	if (remainder > 0)
	{
		memset(padded, 0, 16);
		memcpy(padded, T + tlen - remainder, remainder);
		((__m128i *)&paddedblk)[0] = _mm_loadu_si128(((__m128i *)padded));

		X = _mm512_xor_si512(X, paddedblk);
		X = polydot512(ctx, HH, X);
	}

	remainder = mlen % 16;
	if (mlen >= 16)
	{
		X = polyval512(ctx, X, M, mlen - remainder);
	}

	if (remainder > 0)
	{
		paddedblk = _mm512_setzero_si512();
		memset(padded, 0, 16);
		memcpy(padded, M + mlen - remainder, remainder);
		padded[remainder] = 0x01;
		((__m128i *)&paddedblk)[0] = _mm_loadu_si128(((__m128i *)padded));
		X = _mm512_xor_si512(X, paddedblk);
		X = polydot512(ctx, HH, X);
	}

	return X;
}

static inline __m512i hash512x4(hctr2_context* ctx, uint8_t *M, size_t mlen, uint8_t *T, size_t tlen)
{
	alignas(64) __m512i HH = ctx->hh;
	size_t len = 2 * 8 * tlen + 2;
	if (mlen % 16 != 0)
	{
		len += 1;
	}
	alignas(512) __m512i firstblk = CAST2M512i(len);
	alignas(512) __m512i X = polydot512(ctx, HH, firstblk);
	size_t remainder = tlen % 16;
	if (tlen >= 16)
	{
		X = polyval512x4(ctx, X, T, tlen - remainder);
	}
	uint8_t padded[16];

	alignas(512) __m512i paddedblk = _mm512_setzero_si512();
	if (remainder > 0)
	{
		memset(padded, 0, 16);
		memcpy(padded, T + tlen - remainder, remainder);
		((__m128i *)&paddedblk)[0] = _mm_loadu_si128(((__m128i *)padded));

		X = _mm512_xor_si512(X, paddedblk);
		X = polydot512(ctx, HH, X);
	}

	remainder = mlen % 16;
	if (mlen >= 16)
	{
		X = polyval512x4(ctx, X, M, mlen - remainder);
	}

	if (remainder > 0)
	{
		paddedblk = _mm512_setzero_si512();
		memset(padded, 0, 16);
		memcpy(padded, M + mlen - remainder, remainder);
		padded[remainder] = 0x01;
		((__m128i *)&paddedblk)[0] = _mm_loadu_si128(((__m128i *)padded));
		X = _mm512_xor_si512(X, paddedblk);
		X = polydot512(ctx, HH, X);
	}

	return X;
}
