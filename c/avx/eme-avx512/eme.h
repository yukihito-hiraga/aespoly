#pragma once
#include "../common/graycode.h"
#include "core.h"

static inline __m128i double128(__m128i pp, __m128i X)
{
	alignas(16) __m128i tmp1, tmp2, tmp3, tmp4, tmp5, tmp6;

	tmp1 = _mm_slli_epi32(X, 1);
	tmp2 = _mm_srli_epi32(X, 31);
	tmp3 = _mm_slli_si128(tmp2, 4);
	X = _mm_xor_si128(tmp1, tmp3);
	tmp4 = _mm_srli_si128(tmp2, 12);
	tmp5 = _mm_shuffle_epi8(pp, tmp4);
	X = _mm_xor_si128(X, tmp5);
	return X;
}

static inline __m512i quadruple512(__m512i pp1, __m512i pp2, __m512i X)
{
	alignas(64) __m512i tmp1, tmp2, tmp3, tmp4, tmp5, tmp6;

	tmp1 = _mm512_slli_epi32(X, 4);
	tmp2 = _mm512_srli_epi32(X, 28);
	tmp3 = _mm512_bslli_epi128(tmp2, 4);
	X = _mm512_xor_si512(tmp1, tmp3);
	tmp4 = _mm512_bsrli_epi128(tmp2, 12);
	tmp5 = _mm512_shuffle_epi8(pp1, tmp4);
	tmp6 = _mm512_shuffle_epi8(pp2, tmp4);
	tmp6 = _mm512_bslli_epi128(tmp6, 1);
	X = _mm512_xor_si512(X, _mm512_xor_si512(tmp5, tmp6));
	return X;
}

void emeinit(eme_context *ctx, uint8_t *key)
{
	aesinit128(&(ctx->aesctx), key);
	aesinit512(&(ctx->aesctx), key);

	ctx->poly128 = _mm_setr_epi32(0x87, 0, 0, 0);
	ctx->poly_double128 = _mm_setr_epi32(0x87 << 8, 0, 0, 0);
	ctx->poly_double = _mm512_broadcast_i64x2(ctx->poly_double128);
	ctx->poly = _mm512_broadcast_i64x2(ctx->poly_double128);
	ctx->L[0] = aesenc512(_mm512_setzero_si512(), ctx->aesctx.keys);

	uint8_t chunk[40];
	uint32_t x = 0;
	for (size_t i = 0; i < 16; i++)
	{

		chunk[i] = x & 0xff;
		chunk[i + 16] = (x & 0xff00) >> 8;

		x ^= 0x87;
		if (i % 2 == 1)
		{
			x ^= 0x87 << 1;
		}
		if (i % 4 == 3)
		{
			x ^= 0x87 << 2;
		}
		if (i % 8 == 7)
		{
			x ^= 0x87 << 3;
		}
	}

	ctx->poly_quadruple1 = _mm512_broadcast_i64x2(_mm_loadu_si128((__m128i *)chunk));
	ctx->poly_quadruple2 = _mm512_broadcast_i64x2(_mm_loadu_si128((__m128i *)chunk + 1));

	for (size_t i = 1; i <= 4; i++)
	{
		((__m128i *)ctx->L)[3 + i] = double128(ctx->poly_double128, ((__m128i *)ctx->L)[2 + i]);
	}

	for (size_t i = 2; i < 2900; i++)
	{
		ctx->L[i] = quadruple512(ctx->poly_quadruple1, ctx->poly_quadruple2, ctx->L[i - 1]);
	}
}

static inline __m128i xe(eme_context* ctx, const uint8_t *P, size_t Plen, uint8_t *C)
{
	size_t blen = Plen / 16;
	size_t b512len = blen / 4;
	size_t b512rem = blen % 4;
	size_t b512x4len = b512len / 4;
	size_t b512x4rem = b512len % 4;

	alignas(64) __m512i data[4];
	alignas(64) __m512i tmps[4];

	alignas(64) __m512i sum = _mm512_setzero_si512();

	for (size_t i = 0; i < b512x4len; i++)
	{
		data[0] = _mm512_loadu_si512((__m512i *)P + 4 * i + 0);
		data[1] = _mm512_loadu_si512((__m512i *)P + 4 * i + 1);
		data[2] = _mm512_loadu_si512((__m512i *)P + 4 * i + 2);
		data[3] = _mm512_loadu_si512((__m512i *)P + 4 * i + 3);

		tmps[0] = _mm512_xor_si512(data[0], ctx->L[4 * i + 0 + 1]);
		tmps[1] = _mm512_xor_si512(data[1], ctx->L[4 * i + 1 + 1]);
		tmps[2] = _mm512_xor_si512(data[2], ctx->L[4 * i + 2 + 1]);
		tmps[3] = _mm512_xor_si512(data[3], ctx->L[4 * i + 3 + 1]);

		tmps[0] = aesenc512(tmps[0], ctx->aesctx.keys);
		tmps[1] = aesenc512(tmps[1], ctx->aesctx.keys);
		tmps[2] = aesenc512(tmps[2], ctx->aesctx.keys);
		tmps[3] = aesenc512(tmps[3], ctx->aesctx.keys);

		_mm512_storeu_si512((__m512i *)C + 4 * i + 0, tmps[0]);
		_mm512_storeu_si512((__m512i *)C + 4 * i + 1, tmps[1]);
		_mm512_storeu_si512((__m512i *)C + 4 * i + 2, tmps[2]);
		_mm512_storeu_si512((__m512i *)C + 4 * i + 3, tmps[3]);
		sum = _mm512_xor_si512(_mm512_xor_si512(tmps[0], tmps[1]), _mm512_xor_si512(tmps[2], tmps[3]));
	}

	for (size_t i = b512x4len * 4; i < b512len; i++)
	{
		data[0] = _mm512_loadu_si512((__m512i *)P + i);
		tmps[0] = _mm512_xor_si512(data[0], ctx->L[i + 1]);
		tmps[0] = aesenc512(tmps[0], ctx->aesctx.keys);
		_mm512_storeu_si512((__m512i *)C + i, tmps[0]);
		sum = _mm512_xor_si512(sum, tmps[0]);
	}

	alignas(16) __m128i data128[4];
	alignas(16) __m128i tmps128[4];
	alignas(16) __m128i sum128 = _mm_xor_si128(_mm_xor_si128(((__m128i *)&sum)[0], ((__m128i *)&sum)[1]), _mm_xor_si128(((__m128i *)&sum)[2], ((__m128i *)&sum)[3]));

	for (size_t i = b512len * 4; i < blen; i++)
	{
		data128[0] = _mm_loadu_si128((__m128i *)P + i);
		tmps128[0] = _mm_xor_si128(data128[0], ((__m128i *)ctx->L)[4 + i]);
		tmps128[0] = aesenc128(tmps128[0], ctx->aesctx.keys128);
		sum128 = _mm_xor_si128(sum128, tmps128[0]);
		_mm_storeu_si128((__m128i *)P + i, tmps128[0]);
	}

	return sum128;
}

static inline __m128i middle(eme_context* ctx, __m128i M, const uint8_t *P, size_t Plen, uint8_t *C)
{
	size_t blen = Plen / 16;
	size_t b512len = blen / 4;
	size_t b512rem = blen % 4;
	size_t b512x4len = b512len / 4;
	size_t b512x4rem = b512len % 4;

	alignas(64) __m512i data[4];
	alignas(64) __m512i tmps[4];
	alignas(64) __m512i mask[4];

	alignas(64) __m512i sum = _mm512_setzero_si512();

	((__m128i *)mask)[0] = double128(ctx->poly_double128, M);

	for (size_t i = 1; i < 16; i++)
	{
		((__m128i *)mask)[i] = double128(ctx->poly_double128, ((__m128i *)mask)[i - 1]);
	}

	for (size_t i = 0; i < b512x4len; i++)
	{
		data[0] = _mm512_loadu_si512((__m512i *)P + 4 * i + 0);
		data[1] = _mm512_loadu_si512((__m512i *)P + 4 * i + 1);
		data[2] = _mm512_loadu_si512((__m512i *)P + 4 * i + 2);
		data[3] = _mm512_loadu_si512((__m512i *)P + 4 * i + 3);

		tmps[0] = _mm512_xor_si512(tmps[0], mask[0]);
		tmps[1] = _mm512_xor_si512(tmps[1], mask[1]);
		tmps[2] = _mm512_xor_si512(tmps[2], mask[2]);
		tmps[3] = _mm512_xor_si512(tmps[3], mask[3]);

		mask[0] = quadruple512(ctx->poly_quadruple1, ctx->poly_quadruple2, mask[3]);
		mask[1] = quadruple512(ctx->poly_quadruple1, ctx->poly_quadruple2, mask[0]);
		mask[2] = quadruple512(ctx->poly_quadruple1, ctx->poly_quadruple2, mask[1]);
		mask[3] = quadruple512(ctx->poly_quadruple1, ctx->poly_quadruple2, mask[2]);

		_mm512_storeu_si512((__m512i *)C + 4 * i + 0, tmps[0]);
		_mm512_storeu_si512((__m512i *)C + 4 * i + 1, tmps[1]);
		_mm512_storeu_si512((__m512i *)C + 4 * i + 2, tmps[2]);
		_mm512_storeu_si512((__m512i *)C + 4 * i + 3, tmps[3]);
	}

	for (size_t i = b512x4len * 4; i < b512len; i++)
	{
		data[0] = _mm512_loadu_si512((__m512i *)P + i);
		tmps[0] = _mm512_xor_si512(tmps[0], mask[0]);
		mask[0] = quadruple512(ctx->poly_quadruple1, ctx->poly_quadruple2, mask[0]);
		_mm512_storeu_si512((__m512i *)C + i, tmps[0]);
	}

	alignas(16) __m128i data128[4];
	alignas(16) __m128i tmps128[4];
	alignas(16) __m128i mask128[4];
	alignas(16) __m128i sum128[2];

	sum128[0] = _mm_xor_si128(_mm_xor_si128(((__m128i *)&sum)[0], ((__m128i *)&sum)[1]), _mm_xor_si128(((__m128i *)&sum)[2], ((__m128i *)&sum)[3]));

	mask128[0] = ((__m128i *)mask)[0];

	for (size_t i = b512len * 4; i < blen; i++)
	{
		data128[0] = _mm_loadu_si128((__m128i *)P + i);
		tmps128[0] = _mm_xor_si128(mask128[0], data128[0]);
		sum128[0] = _mm_xor_si128(sum128[0], data128[0]);
		mask128[0] = double128(ctx->poly_double128, mask128[0]);
		_mm_storeu_si128((__m128i *)C + i, tmps128[0]);
	}

	return sum128[0];
}

static inline void ex(eme_context* ctx, const uint8_t *P, size_t Plen, uint8_t *C)
{
	size_t blen = Plen / 16;
	size_t b512len = blen / 4;
	size_t b512rem = blen % 4;
	size_t b512x4len = b512len / 4;
	size_t b512x4rem = b512len % 4;

	alignas(64) __m512i data[4];
	alignas(64) __m512i tmps[4];

	for (size_t i = 0; i < b512x4len; i++)
	{
		data[0] = _mm512_loadu_si512((__m512i *)P + 4 * i + 0);
		data[1] = _mm512_loadu_si512((__m512i *)P + 4 * i + 1);
		data[2] = _mm512_loadu_si512((__m512i *)P + 4 * i + 2);
		data[3] = _mm512_loadu_si512((__m512i *)P + 4 * i + 3);

		tmps[0] = aesenc512(data[0], ctx->aesctx.keys);
		tmps[1] = aesenc512(data[1], ctx->aesctx.keys);
		tmps[2] = aesenc512(data[2], ctx->aesctx.keys);
		tmps[3] = aesenc512(data[3], ctx->aesctx.keys);

		tmps[0] = _mm512_xor_si512(tmps[0], ctx->L[4 * i + 0 + 1]);
		tmps[1] = _mm512_xor_si512(tmps[1], ctx->L[4 * i + 1 + 1]);
		tmps[2] = _mm512_xor_si512(tmps[2], ctx->L[4 * i + 2 + 1]);
		tmps[3] = _mm512_xor_si512(tmps[3], ctx->L[4 * i + 3 + 1]);

		_mm512_storeu_si512((__m512i *)C + 4 * i + 0, tmps[0]);
		_mm512_storeu_si512((__m512i *)C + 4 * i + 1, tmps[1]);
		_mm512_storeu_si512((__m512i *)C + 4 * i + 2, tmps[2]);
		_mm512_storeu_si512((__m512i *)C + 4 * i + 3, tmps[3]);
	}

	for (size_t i = b512x4len * 4; i < b512len; i++)
	{
		data[0] = _mm512_loadu_si512((__m512i *)P + i);
		tmps[0] = aesenc512(data[0], ctx->aesctx.keys);
		tmps[0] = _mm512_xor_si512(tmps[0], ctx->L[i + 1]);

		_mm512_storeu_si512((__m512i *)C + i, tmps[0]);
	}

	alignas(16) __m128i data128[4];
	alignas(16) __m128i tmps128[4];

	for (size_t i = b512len * 4; i < blen; i++)
	{
		data128[0] = _mm_loadu_si128((__m128i *)P + i);
		tmps128[0] = aesenc128(data128[0], ctx->aesctx.keys128);
		tmps128[0] = _mm_xor_si128(tmps128[0], ((__m128i *)ctx->L)[4 + i]);
		_mm_storeu_si128((__m128i *)P + i, tmps128[0]);
	}
}

static inline __m128i eme(eme_context* ctx, uint8_t *T, const uint8_t *P, size_t Plen, uint8_t *C)
{
	alignas(16) __m128i t, sp_ppp1, mp, mc, sc, ccc1;
	t = _mm_loadu_si128((__m128i *)T);
	sp_ppp1 = xe(ctx, P, Plen, C);
	mp = _mm_xor_si128(sp_ppp1, t);
	mc = aesenc128(mp, ctx->aesctx.keys128);
	sc = middle(ctx, _mm_xor_si128(mp, mc), C + 16, Plen - 16, C + 16);
	ccc1 = _mm_xor_si128(mc, _mm_xor_si128(sc, t));
	_mm_storeu_si128((__m128i *)C, ccc1);
	ex(ctx, C, Plen, C);
}