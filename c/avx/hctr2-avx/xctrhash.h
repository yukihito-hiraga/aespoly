#pragma once
#include "../common/common.h"
#include "core.h"
#include "hash.h"

static inline __m128i xctrxoradd_hash128(aes_context aesctx, hctr2_context ctx, __m128i S, size_t mlen, uint8_t *N, uint8_t *C, uint8_t *T, size_t tlen)
{
	alignas(16) __m128i H1 = ctx.htbl[1];

	alignas(16) __m128i ctr = _mm_setr_epi32(1, 0, 0, 0);
	alignas(16) __m128i inc = _mm_setr_epi32(1, 0, 0, 0);

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
		alignas(16) __m128i Y = X;
		alignas(16) __m128i Z = _mm_setzero_si128();
		alignas(16) __m128i data, tmp0, tmp1;
		size_t loopnum = (mlen - remainder) / ctx.blocklength;
		for (size_t i = 0; i < loopnum; i++)
		{
			tmp0 = _mm_xor_si128(ctr, S);
			tmp0 = aesenc128(tmp0, aesctx.keys128);
			data = _mm_loadu_si128((__m128i *)N + i);
			tmp1 = _mm_xor_si128(tmp0, data);
			_mm_storeu_si128((__m128i *)C + i, tmp1);
			ctr = _mm_add_epi64(ctr, inc);

			X = Y;
			X = _mm_xor_si128(X, tmp1);
			Y = polydot128(ctx, X, H1);
		}
	}

	if (remainder > 0)
	{
		alignas(16) __m128i data, tmp0, tmp1;
		size_t loopnum = (mlen - remainder) / ctx.blocklength;
		uint8_t padded[16];
		memset(padded, 0, ctx.blocklength);
		memcpy(padded, N + mlen - remainder, remainder);
		alignas(16) __m128i lastblk = _mm_loadu_si128(((__m128i *)padded));
		tmp0 = aesenc128(_mm_xor_si128(S, _mm_setr_epi64(_mm_set_pi64x((loopnum + 1)), _mm_setzero_si64())), aesctx.keys128);
		lastblk = _mm_xor_si128(lastblk, tmp0);
		memset(((uint8_t *)&lastblk) + remainder, 0, ctx.blocklength - remainder);
		memcpy(C + mlen - remainder, ((uint8_t *)&lastblk), remainder);

		((uint8_t *)&lastblk)[remainder] = 0x01;
		X = _mm_xor_si128(X, lastblk);
		X = polydot128(ctx, H1, X);
	}

	return X;
}

static inline __m128i xctrxoradd_hash128x4(aes_context aesctx, hctr2_context ctx, __m128i S, size_t mlen, uint8_t *N, uint8_t *C, uint8_t *T, size_t tlen)
{

	alignas(16) __m128i ctr[4] = {_mm_setr_epi32(1, 0, 0, 0),
								  _mm_setr_epi32(2, 0, 0, 0),
								  _mm_setr_epi32(3, 0, 0, 0),
								  _mm_setr_epi32(4, 0, 0, 0)};
	alignas(16) __m128i inc = _mm_setr_epi32(4, 0, 0, 0);

	size_t len = 2 * 8 * tlen + 2;
	if (mlen % ctx.blocklength != 0)
	{
		len += 1;
	}
	alignas(16) __m128i firstblk = _mm_setr_epi64(_mm_set_pi64x(len), _mm_setzero_si64());
	alignas(16) __m128i X = polydot128(ctx, ctx.htbl[1], firstblk);
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
		X = polydot128(ctx, ctx.htbl[1], X);
	}

	remainder = mlen % ctx.blocklength;
	if (mlen >= ctx.blocklength)
	{

		alignas(16) __m128i tmps[4];

		alignas(16) __m128i Y = _mm_setzero_si128(), Z = _mm_setzero_si128();
		alignas(16) __m128i data[4];

		size_t loopnum = (mlen - remainder) / ctx.blocklength;
		for (size_t i = 0; i < (loopnum / 4); i++)
		{
			loadx4((__m128i *)N + i * 4, data);

			tmps[0] = _mm_xor_si128(ctr[0], S);
			tmps[1] = _mm_xor_si128(ctr[1], S);
			tmps[2] = _mm_xor_si128(ctr[2], S);
			tmps[3] = _mm_xor_si128(ctr[3], S);

			tmps[0] = aesenc128(tmps[0], aesctx.keys128);
			tmps[1] = aesenc128(tmps[1], aesctx.keys128);
			tmps[2] = aesenc128(tmps[2], aesctx.keys128);
			tmps[3] = aesenc128(tmps[3], aesctx.keys128);

			data[0] = _mm_xor_si128(tmps[0], data[0]);
			data[1] = _mm_xor_si128(tmps[1], data[1]);
			data[2] = _mm_xor_si128(tmps[2], data[2]);
			data[3] = _mm_xor_si128(tmps[3], data[3]);

			storex4((__m128i *)C + i * 4, data);

			Y = polyreduce128(ctx, Y);
			Z = _mm_xor_si128(X, Y);

			Z = _mm_xor_si128(Z, data[0]);

			schoolbook_initialadd128(data[3], ctx.htbl[1], tmps);
			schoolbook_add128(data[2], ctx.htbl[2], tmps);
			schoolbook_add128(data[1], ctx.htbl[3], tmps);
			schoolbook_add128(Z, ctx.htbl[4], tmps);

			tmps[3] = _mm_bsrli_si128(tmps[2], 8);
			tmps[2] = _mm_bslli_si128(tmps[2], 8);

			X = _mm_xor_si128(tmps[3], tmps[1]);
			Y = _mm_xor_si128(tmps[0], tmps[2]);

			ctr[0] = _mm_add_epi64(ctr[0], inc);
			ctr[1] = _mm_add_epi64(ctr[1], inc);
			ctr[2] = _mm_add_epi64(ctr[2], inc);
			ctr[3] = _mm_add_epi64(ctr[3], inc);
		}
		Y = polyreduce128(ctx, Y);
		Z = _mm_xor_si128(X, Y);
		X = Z;
		inc = _mm_setr_epi32(1, 0, 0, 0);
		for (size_t i = 0; i < (loopnum % 8); i++)
		{
			data[0] = _mm_loadu_si128((__m128i *)N + loopnum - (loopnum % 8) + i);
			tmps[0] = _mm_xor_si128(ctr[0], S);
			tmps[0] = aesenc128(tmps[0], aesctx.keys128);
			tmps[1] = _mm_xor_si128(tmps[0], data[0]);
			_mm_storeu_si128((__m128i *)C + loopnum - (loopnum % 8) + i, tmps[1]);
			X = _mm_xor_si128(X, tmps[1]);
			X = polydot128(ctx, X, ctx.htbl[1]);
			ctr[0] = _mm_add_epi64(ctr[0], inc);
		}
	}

	if (remainder > 0)
	{
		alignas(16) __m128i data, tmp0, tmp1;
		size_t loopnum = (mlen - remainder) / ctx.blocklength;
		uint8_t padded[16];
		memset(padded, 0, ctx.blocklength);
		memcpy(padded, N + mlen - remainder, remainder);
		alignas(16) __m128i lastblk = _mm_loadu_si128(((__m128i *)padded));
		tmp0 = aesenc128(_mm_xor_si128(S, _mm_setr_epi64(_mm_set_pi64x((loopnum + 1)), _mm_setzero_si64())), aesctx.keys128);
		lastblk = _mm_xor_si128(lastblk, tmp0);
		memset(((uint8_t *)&lastblk) + remainder, 0, ctx.blocklength - remainder);
		memcpy(C + mlen - remainder, ((uint8_t *)&lastblk), remainder);

		((uint8_t *)&lastblk)[remainder] = 0x01;
		X = _mm_xor_si128(X, lastblk);
		X = polydot128(ctx, ctx.htbl[1], X);
	}

	return X;
}

static inline __m128i xctrxoradd_hash128x8(aes_context aesctx, hctr2_context ctx, __m128i S, size_t mlen, uint8_t *N, uint8_t *C, uint8_t *T, size_t tlen)
{

	alignas(16) __m128i ctr[8];
	for (size_t i = 0; i < 8; i++)
	{
		ctr[i] = _mm_setr_epi32(i + 1, 0, 0, 0);
	}
	alignas(16) __m128i inc = _mm_setr_epi32(8, 0, 0, 0);

	size_t len = 2 * 8 * tlen + 2;
	if (mlen % ctx.blocklength != 0)
	{
		len += 1;
	}
	alignas(16) __m128i firstblk = _mm_setr_epi64(_mm_set_pi64x(len), _mm_setzero_si64());
	alignas(16) __m128i X = polydot128(ctx, ctx.htbl[1], firstblk);
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
		X = polydot128(ctx, ctx.htbl[1], X);
	}

	remainder = mlen % ctx.blocklength;
	if (mlen >= ctx.blocklength)
	{

		alignas(16) __m128i tmps[16];

		alignas(16) __m128i Y = _mm_setzero_si128(), Z = _mm_setzero_si128();
		alignas(16) __m128i data[8];

		__m128i *ttmps = tmps + 12;

		size_t loopnum = (mlen - remainder) / ctx.blocklength;
		for (size_t i = 0; i < (loopnum / 8); i++)
		{
			loadx8((__m128i *)N + 8 * i, data);

			xorx8_bfix(ctr, S, tmps);
			tmps[0] = aesenc128(tmps[0], aesctx.keys128);
			tmps[1] = aesenc128(tmps[1], aesctx.keys128);
			tmps[2] = aesenc128(tmps[2], aesctx.keys128);
			tmps[3] = aesenc128(tmps[3], aesctx.keys128);
			tmps[4] = aesenc128(tmps[4], aesctx.keys128);
			tmps[5] = aesenc128(tmps[5], aesctx.keys128);
			tmps[6] = aesenc128(tmps[6], aesctx.keys128);
			tmps[7] = aesenc128(tmps[7], aesctx.keys128);
			xorx8_1wise(data, tmps, tmps);
			storex8((__m128i *)C + 8 * i, tmps);

			Y = polyreduce128(ctx, Y);
			Z = _mm_xor_si128(X, Y);
			Z = _mm_xor_si128(Z, tmps[0]);
			schoolbook_initialadd128(tmps[7], ctx.htbl[1], ttmps);
			schoolbook_add128(tmps[6], ctx.htbl[2], ttmps);
			schoolbook_add128(tmps[5], ctx.htbl[3], ttmps);
			schoolbook_add128(tmps[4], ctx.htbl[4], ttmps);
			schoolbook_add128(tmps[3], ctx.htbl[5], ttmps);
			schoolbook_add128(tmps[2], ctx.htbl[6], ttmps);
			schoolbook_add128(tmps[1], ctx.htbl[7], ttmps);
			schoolbook_add128(Z, ctx.htbl[8], ttmps);
			ttmps[3] = _mm_bsrli_si128(ttmps[2], 8);
			ttmps[2] = _mm_bslli_si128(ttmps[2], 8);
			X = _mm_xor_si128(ttmps[3], ttmps[1]);
			Y = _mm_xor_si128(ttmps[0], ttmps[2]);

			addx8_bfix(ctr, inc, ctr);
		}
		Y = polyreduce128(ctx, Y);
		Z = _mm_xor_si128(X, Y);
		X = Z;
		inc = _mm_setr_epi32(1, 0, 0, 0);
		for (size_t i = 0; i < (loopnum % 4); i++)
		{
			data[0] = _mm_loadu_si128((__m128i *)N + loopnum - (loopnum % 4) + i);
			tmps[0] = _mm_xor_si128(ctr[0], S);
			tmps[0] = aesenc128(tmps[0], aesctx.keys128);
			tmps[1] = _mm_xor_si128(tmps[0], data[0]);
			_mm_storeu_si128((__m128i *)C + loopnum - (loopnum % 4) + i, tmps[1]);
			X = _mm_xor_si128(X, tmps[1]);
			X = polydot128(ctx, X, ctx.htbl[1]);
			ctr[0] = _mm_add_epi64(ctr[0], inc);
		}
	}

	if (remainder > 0)
	{
		alignas(16) __m128i data, tmp0, tmp1;
		size_t loopnum = (mlen - remainder) / ctx.blocklength;
		uint8_t padded[16];
		memset(padded, 0, ctx.blocklength);
		memcpy(padded, N + mlen - remainder, remainder);
		alignas(16) __m128i lastblk = _mm_loadu_si128(((__m128i *)padded));
		tmp0 = aesenc128(_mm_xor_si128(S, _mm_setr_epi64(_mm_set_pi64x((loopnum + 1)), _mm_setzero_si64())), aesctx.keys128);
		lastblk = _mm_xor_si128(lastblk, tmp0);
		memset(((uint8_t *)&lastblk) + remainder, 0, ctx.blocklength - remainder);
		memcpy(C + mlen - remainder, ((uint8_t *)&lastblk), remainder);

		((uint8_t *)&lastblk)[remainder] = 0x01;
		X = _mm_xor_si128(X, lastblk);
		X = polydot128(ctx, ctx.htbl[1], X);
	}

	return X;
}
