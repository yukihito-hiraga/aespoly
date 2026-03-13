#pragma once
#include "../common/common.h"
#include "core.h"
#include "hash.h"

static inline __m512i xctrxoradd_hash512(aes_context* aesctx, hctr2_context* ctx, __m512i S, size_t mlen, uint8_t *N, uint8_t *C, uint8_t *T, size_t tlen)
{
	size_t len = 2 * 8 * tlen + 2;
	if (mlen % ctx->blocklength != 0)
	{
		len += 1;
	}
	alignas(512) __m512i firstblk = CAST2M512i(len);
	alignas(512) __m512i X = polydot512(ctx, ctx->hh, firstblk);
	size_t remainder = tlen % ctx->blocklength;
	if (tlen >= ctx->blocklength)
	{
		X = polyval512(ctx, X, T, tlen - remainder);
	}
	uint8_t padded[16];
	alignas(512) __m512i paddedblk = _mm512_setzero_si512();
	if (remainder > 0)
	{
		memset(padded, 0, ctx->blocklength);
		memcpy(padded, T + tlen - remainder, remainder);
		((__m128i *)&paddedblk)[0] = _mm_loadu_si128(((__m128i *)padded));

		X = _mm512_xor_si512(X, paddedblk);
		X = polydot512(ctx, ctx->hh, X);
	}

	alignas(512) __m512i Y;
	alignas(512) __m512i Hx4 = ctx->hx4tbl[0];
	paddedblk = _mm512_setzero_si512();
	alignas(512) __m512i H0 = ctx->htbl[0];
	alignas(512) __m512i data;

	alignas(512) __m512i tmp0, tmp1;

	remainder = mlen % ctx->blocklength;
	size_t ln = mlen / ctx->blocklength;

	size_t remainder_roll = ln % 4;

	alignas(512) __m512i ctr = _mm512_setr_epi64(1, 0, 2, 0, 3, 0, 4, 0);
	alignas(512) __m512i one1 = _mm512_setr4_epi32(1, 0, 0, 0);
	alignas(512) __m512i inc = _mm512_setr4_epi32(4, 0, 0, 0);

	alignas(128) __m128i tmp;
	tmp = ((__m128i *)&X)[0];
	Y = _mm512_setzero_si512();
	((__m128i *)&Y)[0] = tmp;
	for (size_t i = 0; i < (ln / 4); i++)
	{
		tmp0 = _mm512_xor_si512(ctr, S);
		tmp0 = aesenc512(tmp0, aesctx->keys);
		tmp1 = _mm512_loadu_si512(N + i * 64);

		tmp1 = _mm512_xor_si512(tmp0, tmp1);
		_mm512_storeu_si512(C + i * 64, tmp1);

		X = Y;
		X = _mm512_xor_si512(X, tmp1);
		Y = polydot512(ctx, X, Hx4);

		ctr = _mm512_add_epi64(ctr, inc);
	}
	if (ln >= 4)
	{
		X = polydot512(ctx, X, H0);
	}

	((__m128i *)&tmp0)[0] = ((__m128i *)&X)[2];
	((__m128i *)&tmp0)[1] = ((__m128i *)&X)[3];
	tmp0 = _mm512_xor_si512(X, tmp0);
	X = _mm512_setzero_si512();
	((__m128i *)&X)[0] = _mm_xor_si128(((__m128i *)&tmp0)[0], ((__m128i *)&tmp0)[1]);

	for (size_t i = 0; i < remainder_roll; i++)
	{
		tmp0 = _mm512_xor_si512(ctr, S);
		tmp0 = aesenc512(tmp0, aesctx->keys);

		((__m128i *)&tmp1)[0] = ((__m128i *)N)[ln - remainder_roll + i];
		tmp1 = _mm512_xor_si512(tmp0, tmp1);
		((__m128i *)C)[ln - remainder_roll + i] = ((__m128i *)&tmp1)[0];
		ctr = _mm512_add_epi64(ctr, one1);

		data = _mm512_setzero_si512();
		((__m128i *)&data)[0] = ((__m128i *)&tmp1)[0];
		X = _mm512_xor_si512(X, data);
		X = polydot512(ctx, X, ctx->hh);
	}

	if (remainder > 0)
	{
		tmp0 = _mm512_xor_si512(ctr, S);
		tmp0 = aesenc512(tmp0, aesctx->keys);

		uint8_t padded[16];
		memset(padded, 0, ctx->blocklength);
		memcpy(padded, N + mlen - remainder, remainder);
		padded[remainder] = 0x01;

		paddedblk = _mm512_setzero_si512();
		((__m128i *)&paddedblk)[0] = _mm_loadu_si128((__m128i *)padded);
		paddedblk = _mm512_xor_si512(paddedblk, tmp0);

		_mm_storeu_si128((__m128i *)&padded, ((__m128i *)&paddedblk)[0]);
		memset(padded + remainder, 0, ctx->blocklength - remainder);
		memcpy(C + mlen - remainder, padded, remainder);
		padded[remainder] = 0x01;

		paddedblk = _mm512_setzero_si512();
		((__m128i *)&paddedblk)[0] = _mm_loadu_si128((__m128i *)padded);
		X = _mm512_xor_si512(X, paddedblk);
		X = polydot512(ctx, ctx->hh, X);
	}

	((__m128i *)&data)[0] = ((__m128i *)&X)[2];
	((__m128i *)&data)[1] = ((__m128i *)&X)[3];
	X = _mm512_xor_si512(X, data);
	((__m128i *)&X)[0] = _mm_xor_si128(((__m128i *)&X)[0], ((__m128i *)&X)[1]);

	return X;
}

static inline __m512i xctrxoradd_hash512x4(aes_context* aesctx, hctr2_context* ctx, __m512i SS, size_t mlen, uint8_t *N, uint8_t *C, uint8_t *T, size_t tlen)
{
	size_t len = 2 * 8 * tlen + 2;
	if (mlen % ctx->blocklength != 0)
	{
		len += 1;
	}
	alignas(512) __m512i firstblk = CAST2M512i(len);
	alignas(512) __m512i X = polydot512(ctx, ctx->hh, firstblk);
	size_t remainder = tlen % ctx->blocklength;
	if (tlen >= ctx->blocklength)
	{
		X = polyval512x4(ctx, X, T, tlen - remainder);
	}
	uint8_t padded[16];

	alignas(512) __m512i paddedblk = _mm512_setzero_si512();
	if (remainder > 0)
	{
		memset(padded, 0, ctx->blocklength);
		memcpy(padded, T + tlen - remainder, remainder);
		((__m128i *)&paddedblk)[0] = _mm_loadu_si128(((__m128i *)padded));

		X = _mm512_xor_si512(X, paddedblk);
		X = polydot512(ctx, ctx->hh, X);
	}

	alignas(512) __m512i tmps[8];
	__m512i* tmps1 = tmps;
	__m512i* tmps2 = tmps+4;

	alignas(512) __m512i Y = _mm512_setzero_si512(), Z = _mm512_setzero_si512();
	alignas(512) __m512i data[4];

	paddedblk = _mm512_setzero_si512();

	remainder = mlen % ctx->blocklength;
	size_t ln = mlen / ctx->blocklength;

	size_t remainder_roll = ln % (4 * 4);

	alignas(64) __m512i ctr[4];
	for (size_t i = 0; i < 16; i++)
	{
		((__m128i *)(&ctr))[i] = _mm_setr_epi32(i + 1, 0, 0, 0);
	}
	alignas(64) __m512i inc16 = _mm512_broadcast_i64x2(_mm_setr_epi32(16, 0, 0, 0));
	alignas(512) __m512i one1 = _mm512_setr4_epi32(1, 0, 0, 0);
	alignas(512) __m512i inc = _mm512_setr4_epi32(16, 0, 0, 0);

	alignas(128) __m128i tmp;
	tmp = ((__m128i *)&X)[0];
	X = _mm512_setzero_si512();
	((__m128i *)&X)[0] = tmp;

	for (size_t i = 0; i < (ln / (4 * 4)); i++)
	{
		data[0] = _mm512_loadu_si512((__m512 *)N + (i * 4 + 0));
		data[1] = _mm512_loadu_si512((__m512 *)N + (i * 4 + 1));
		data[2] = _mm512_loadu_si512((__m512 *)N + (i * 4 + 2));
		data[3] = _mm512_loadu_si512((__m512 *)N + (i * 4 + 3));

		tmps1[0] = _mm512_xor_si512(ctr[0], SS);
		tmps1[1] = _mm512_xor_si512(ctr[1], SS);
		tmps1[2] = _mm512_xor_si512(ctr[2], SS);
		tmps1[3] = _mm512_xor_si512(ctr[3], SS);

		tmps1[0] = aesenc512(tmps1[0], aesctx->keys);
		tmps1[1] = aesenc512(tmps1[1], aesctx->keys);
		tmps1[2] = aesenc512(tmps1[2], aesctx->keys);
		tmps1[3] = aesenc512(tmps1[3], aesctx->keys);

		tmps1[0] = _mm512_xor_si512(tmps1[0], data[0]);
		tmps1[1] = _mm512_xor_si512(tmps1[1], data[1]);
		tmps1[2] = _mm512_xor_si512(tmps1[2], data[2]);
		tmps1[3] = _mm512_xor_si512(tmps1[3], data[3]);

		_mm512_storeu_si512((__m512i *)C + 4 * i + 0, tmps1[0]);
		_mm512_storeu_si512((__m512i *)C + 4 * i + 1, tmps1[1]);
		_mm512_storeu_si512((__m512i *)C + 4 * i + 2, tmps1[2]);
		_mm512_storeu_si512((__m512i *)C + 4 * i + 3, tmps1[3]);

		Y = polyreduce512(ctx, Y);
		Z = _mm512_xor_si512(X, Y);

		Z = _mm512_xor_si512(Z, tmps1[0]);

		schoolbook_initialadd512(tmps2, tmps1[3], ctx->htbl[0]);
		schoolbook_add512(tmps2, tmps1[2], ctx->htbl[1]);
		schoolbook_add512(tmps2, tmps1[1], ctx->htbl[2]);
		schoolbook_add512(tmps2, Z, ctx->htbl[3]);

		tmps2[3] = _mm512_bsrli_epi128(tmps2[2], 8);
		tmps2[2] = _mm512_bslli_epi128(tmps2[2], 8);

		X = _mm512_xor_si512(tmps2[3], tmps2[1]);
		Y = _mm512_xor_si512(tmps2[0], tmps2[2]);

		ctr[0] = _mm512_add_epi64(ctr[0], inc16);
		ctr[1] = _mm512_add_epi64(ctr[1], inc16);
		ctr[2] = _mm512_add_epi64(ctr[2], inc16);
		ctr[3] = _mm512_add_epi64(ctr[3], inc16);
	}

	Y = polyreduce512(ctx, Y);
	Z = _mm512_xor_si512(X, Y);
	X = Z;
	((__m128i *)&tmps)[0] = ((__m128i *)&Z)[2];
	((__m128i *)&tmps)[1] = ((__m128i *)&Z)[3];
	tmps[0] = _mm512_xor_si512(Z, tmps[0]);
	X = tmps[0];

	
	for (size_t i = 0; i < remainder_roll; i++)
	{
		tmps[0] = _mm512_xor_si512(ctr[0], SS);
		tmps[0] = aesenc512(tmps[0], aesctx->keys);

		((__m128i *)&tmps[1])[0] = ((__m128i *)N)[ln - remainder_roll + i];
		tmps[1] = _mm512_xor_si512(tmps[0], tmps[1]);
		((__m128i *)C)[ln - remainder_roll + i] = ((__m128i *)&tmps[1])[0];
		ctr[0] = _mm512_add_epi64(ctr[0], one1);

		data[0] = _mm512_setzero_si512();
		((__m128i *)&data)[0] = ((__m128i *)&tmps[1])[0];
		X = _mm512_xor_si512(X, data[0]);
		X = polydot512(ctx, X, ctx->hh);
	}
	if (remainder > 0)
	{
		tmps[0] = _mm512_xor_si512(ctr[0], SS);
		tmps[0] = aesenc512(tmps[0], aesctx->keys);

		uint8_t padded[16];
		memset(padded, 0, ctx->blocklength);
		memcpy(padded, N + mlen - remainder, remainder);
		padded[remainder] = 0x01;

		paddedblk = _mm512_setzero_si512();
		((__m128i *)&paddedblk)[0] = _mm_loadu_si128((__m128i *)padded);
		paddedblk = _mm512_xor_si512(paddedblk, tmps[0]);

		_mm_storeu_si128((__m128i *)&padded, ((__m128i *)&paddedblk)[0]);
		memset(padded + remainder, 0, ctx->blocklength - remainder);
		memcpy(C + mlen - remainder, padded, remainder);
		padded[remainder] = 0x01;

		paddedblk = _mm512_setzero_si512();
		((__m128i *)&paddedblk)[0] = _mm_loadu_si128((__m128i *)padded);

		X = _mm512_xor_si512(X, paddedblk);
		X = polydot512(ctx, ctx->hh, X);
	}

	((__m128i *)&data)[0] = ((__m128i *)&X)[2];
	((__m128i *)&data)[1] = ((__m128i *)&X)[3];
	X = _mm512_xor_si512(X, data[0]);
	((__m128i *)&X)[0] = _mm_xor_si128(((__m128i *)&X)[0], ((__m128i *)&X)[1]);

	return X;
}