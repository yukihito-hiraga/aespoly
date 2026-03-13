#pragma once
#include "../common/common.h"

#include "core.h"

static inline __m128i polyreduce128(hctr2_context ctx, __m128i x)
{
	alignas(16) __m128i x0 = _mm_clmulepi64_si128(x, ctx.poly, 0x10);
	alignas(16) __m128i y0 = _mm_shuffle_epi32(x, 78);
	alignas(16) __m128i y1 = _mm_xor_si128(y0, x0);
	alignas(16) __m128i x1 = _mm_clmulepi64_si128(y1, ctx.poly, 0x10);
	alignas(16) __m128i y2 = _mm_shuffle_epi32(y1, 78);
	return _mm_xor_si128(y2, x1);
}

static inline __m128i polydot128(hctr2_context ctx, __m128i a, __m128i b)
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
	alignas(16) __m128i ppl_reduced = polyreduce128(ctx, pplower);
	return _mm_xor_si128(ppupper, ppl_reduced);
}

// required blen to be divisable by blocklength
static inline __m128i polyval128(hctr2_context ctx, __m128i state, uint8_t *blks, size_t blen)
{
	alignas(16) __m128i H0 = ctx.htbl[1];
	alignas(16) __m128i X = state;
	alignas(16) __m128i data, tmp0, tmp1;
	size_t loopnum = blen / ctx.blocklength;
	for (size_t i = 0; i < loopnum; i++)
	{
		data = _mm_loadu_si128((__m128i*)blks + i);
		X = _mm_xor_si128(X, data);
		X = polydot128(ctx, X, H0);
	}

	return X;
}

static inline __m128i polyval128x4(hctr2_context ctx, __m128i state, uint8_t *blks, size_t blen)
{

	alignas(16) __m128i tmps[4];

	alignas(16) __m128i X = state;
	alignas(16) __m128i Y = _mm_setzero_si128(), Z = _mm_setzero_si128();
	alignas(16) __m128i data[4];

	size_t loopnum = blen / ctx.blocklength;
	size_t remainder = loopnum % 4;
	for (size_t i = 0; i < (loopnum / 4); i++)
	{
		loadx4((__m128i*)blks + i*4, data);

		Y = polyreduce128(ctx, Y);
		Z = _mm_xor_si128(X, Y);
		Z = _mm_xor_si128(Z, data[0]);

		schoolbook_initialadd128(data[3], ctx.htbl[0], tmps);

		schoolbook_add128(data[2], ctx.htbl[1], tmps);
		schoolbook_add128(data[1], ctx.htbl[2], tmps);
		schoolbook_add128(Z, ctx.htbl[3], tmps);

		tmps[3] = _mm_bsrli_si128(tmps[2], 8);
		tmps[2] = _mm_bslli_si128(tmps[2], 8);

		X = _mm_xor_si128(tmps[3], tmps[1]);
		Y = _mm_xor_si128(tmps[0], tmps[2]);
	}
	Y = polyreduce128(ctx, Y);
	Z = _mm_xor_si128(X, Y);
	X = Z;
	for (size_t i = 0; i < remainder; i++)
	{
		data[0] = _mm_loadu_si128((__m128i*)blks + loopnum - remainder + i);
		X = _mm_xor_si128(X, data[0]);
		X = polydot128(ctx, X, ctx.htbl[0]);
	}
	return X;
}

static inline __m128i polyval128x8(hctr2_context ctx, __m128i state, uint8_t *P, size_t Plen)
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

		Y = polyreduce128(ctx, Y);
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
	Y = polyreduce128(ctx, Y);
	Z = _mm_xor_si128(X, Y);
	X = Z;
	for (size_t i = 0; i < remainder; i++)
	{
		data[0] = _mm_loadu_si128((__m128i *)P + loopnum - remainder + i);
		X = _mm_xor_si128(X, data[0]);
		X = polydot128(ctx, X, ctx.htbl[0]);
	}
	return X;
}

static inline __m128i hash128(hctr2_context ctx, uint8_t *M, size_t mlen, uint8_t *T, size_t tlen)
{
	alignas(16) __m128i H1 = ctx.htbl[1];
	size_t len = 2 * 8 * tlen + 2;
	if (mlen % ctx.blocklength != 0)
	{
		len += 1;
	}
	alignas(16) __m128i firstblk = _mm_setr_epi64(_mm_set_pi64x(len), _mm_setzero_si64());
	alignas(16) __m128i X = polydot128(ctx, H1, firstblk);
	size_t remainder = tlen % ctx.blocklength;
	if (tlen >= ctx.blocklength)
	{
		X = polyval128(ctx, X, T, tlen - remainder);
	}
	uint8_t padded[16];
	alignas(16) __m128i paddedblk = _mm_setzero_si128();
	if (remainder > 0)
	{
		memset(padded, 0, ctx.blocklength);
		memcpy(padded, T + tlen - remainder, remainder);
		paddedblk = _mm_loadu_si128(((__m128i *)padded));

		X = _mm_xor_si128(X, paddedblk);
		X = polydot128(ctx, H1, X);
	}

	remainder = mlen % ctx.blocklength;
	if (mlen >= ctx.blocklength)
	{
		X = polyval128(ctx, X, M, mlen - remainder);
	}

	if (remainder > 0)
	{
		paddedblk = _mm_setzero_si128();
		memset(padded, 0, ctx.blocklength);
		memcpy(padded, M + mlen - remainder, remainder);
		padded[remainder] = 0x01;
		paddedblk = _mm_loadu_si128(((__m128i *)padded));
		X = _mm_xor_si128(X, paddedblk);
		X = polydot128(ctx, H1, X);
	}

	return X;
}

static inline __m128i hash128x4(hctr2_context ctx, uint8_t *M, size_t mlen, uint8_t *T, size_t tlen)
{
	alignas(16) __m128i H1 = ctx.htbl[1];
	size_t len = 2 * 8 * tlen + 2;
	if (mlen % ctx.blocklength != 0)
	{
		len += 1;
	}
	alignas(16) __m128i firstblk = _mm_setr_epi64(_mm_set_pi64x(len), _mm_setzero_si64());
	alignas(16) __m128i X = polydot128(ctx, H1, firstblk);
	size_t remainder = tlen % ctx.blocklength;
	if (tlen >= ctx.blocklength)
	{
		X = polyval128x4(ctx, X, T, tlen - remainder);
	}
	uint8_t padded[16];
	alignas(16) __m128i paddedblk = _mm_setzero_si128();
	if (remainder > 0)
	{
		memset(padded, 0, ctx.blocklength);
		memcpy(padded, T + tlen - remainder, remainder);
		paddedblk = _mm_loadu_si128(((__m128i *)padded));

		X = _mm_xor_si128(X, paddedblk);
		X = polydot128(ctx, H1, X);
	}

	remainder = mlen % ctx.blocklength;
	if (mlen >= ctx.blocklength)
	{
		X = polyval128x4(ctx, X, M, mlen - remainder);
	}

	if (remainder > 0)
	{
		paddedblk = _mm_setzero_si128();
		memset(padded, 0, ctx.blocklength);
		memcpy(padded, M + mlen - remainder, remainder);
		padded[remainder] = 0x01;
		paddedblk = _mm_loadu_si128(((__m128i *)padded));
		X = _mm_xor_si128(X, paddedblk);
		X = polydot128(ctx, H1, X);
	}

	return X;
}


static inline __m128i hash128x8(hctr2_context ctx, uint8_t *M, size_t mlen, uint8_t *T, size_t tlen)
{
	alignas(16) __m128i H1 = ctx.htbl[1];
	size_t len = 2 * 8 * tlen + 2;
	if (mlen % ctx.blocklength != 0)
	{
		len += 1;
	}
	alignas(16) __m128i firstblk = _mm_setr_epi64(_mm_set_pi64x(len), _mm_setzero_si64());
	alignas(16) __m128i X = polydot128(ctx, H1, firstblk);
	size_t remainder = tlen % ctx.blocklength;
	if (tlen >= ctx.blocklength)
	{
		X = polyval128x8(ctx, X, T, tlen - remainder);
	}
	uint8_t padded[16];
	alignas(16) __m128i paddedblk = _mm_setzero_si128();
	if (remainder > 0)
	{
		memset(padded, 0, ctx.blocklength);
		memcpy(padded, T + tlen - remainder, remainder);
		paddedblk = _mm_loadu_si128(((__m128i *)padded));

		X = _mm_xor_si128(X, paddedblk);
		X = polydot128(ctx, H1, X);
	}

	remainder = mlen % ctx.blocklength;
	if (mlen >= ctx.blocklength)
	{
		X = polyval128x8(ctx, X, M, mlen - remainder);
	}

	if (remainder > 0)
	{
		paddedblk = _mm_setzero_si128();
		memset(padded, 0, ctx.blocklength);
		memcpy(padded, M + mlen - remainder, remainder);
		padded[remainder] = 0x01;
		paddedblk = _mm_loadu_si128(((__m128i *)padded));
		X = _mm_xor_si128(X, paddedblk);
		X = polydot128(ctx, H1, X);
	}

	return X;
}