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

static inline __m128i mul2_128(__m128i pp, __m128i X)
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

static inline __m128i mul16_128(__m128i pp1, __m128i pp2, __m128i X)
{
	alignas(16) __m128i tmp1, tmp2, tmp3, tmp4, tmp5, tmp6;

	tmp1 = _mm_slli_epi32(X, 4);
	tmp2 = _mm_srli_epi32(X, 28);
	tmp3 = _mm_slli_si128(tmp2, 4);
	X = _mm_xor_si128(tmp1, tmp3);
	tmp4 = _mm_srli_si128(tmp2, 12);
	tmp5 = _mm_shuffle_epi8(pp1, tmp4);
	tmp6 = _mm_shuffle_epi8(pp2, tmp4);
	tmp6 = _mm_slli_si128(tmp6, 1);
	X = _mm_xor_si128(X, _mm_xor_si128(tmp5, tmp6));
	return X;
}

#define mul2rev_128(pp, X) byterev(mul2_128(pp, byterev(X)))
#define mul16rev_128(pp1, pp2, X) byterev(mul16_128(pp1, pp2, byterev(X)))
#define L128_PTR(ctx) (((__m128i *)((ctx)->L)) + 3)

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
	__m128i *L = L128_PTR(ctx);
	L[0] = aesenc128(_mm_setzero_si128(), ctx->aesctx.keys128);
	for (size_t i = 0; i < 4095; i++)
	{
		L[i + 1] = mul2rev_128(ctx->poly_double128, L[i]);
	}
}

static inline __m128i xe(eme_context* ctx, const uint8_t *P, size_t Plen, uint8_t *C)
{
	__m128i *L = L128_PTR(ctx);
	size_t blen = Plen / 16;
	size_t bx8len = blen / 8;
	size_t bx8rem = blen % 8;

	alignas(16) __m128i data[8];
	alignas(16) __m128i tmps[8];
	__m128i sum = _mm_setzero_si128();

	for (size_t i = 0; i < bx8len; i++)
	{
		loadx8((__m128i *)P + 8 * i, data);
		xorx8_1wise(L + 8 * i, data, tmps);
		aesx8(ctx->aesctx.keys128, tmps, tmps);
		storex8((__m128i *)C + 8 * i, tmps);
		sum_x8(tmps, sum);
	}

	for (size_t i = 0; i < bx8rem; i++)
	{
		data[0] = _mm_loadu_si128((__m128i *)P + 8 * bx8len + i);
		tmps[0] = _mm_xor_si128(L[8 * bx8len + i], data[0]);
		tmps[0] = aesenc128(tmps[0], ctx->aesctx.keys128);
		_mm_store_si128((__m128i *)C + 8 * bx8len + i, tmps[0]);
		sum = _mm_xor_si128(sum, tmps[0]);
	}

	return sum;
}

static inline __m128i middle(eme_context* ctx, __m128i M, const uint8_t *P, size_t Plen, uint8_t *C)
{
	__m128i *pp1 = (__m128i *)&ctx->poly_quadruple1;
	__m128i *pp2 = (__m128i *)&ctx->poly_quadruple2;
	size_t blen = Plen / 16;
	size_t bx8len = blen / 8;
	size_t bx8rem = blen % 8;

	alignas(16) __m128i data[8];
	alignas(16) __m128i tmps[8];
	alignas(16) __m128i mask[8];
	__m128i sum = _mm_setzero_si128();

	mask[0] = mul2rev_128(ctx->poly_double128, M);
	for (size_t i = 1; i < 8; i++)
	{
		mask[i] = mul2rev_128(ctx->poly_double128, mask[i - 1]);
	}

	for (size_t i = 0; i < bx8len; i++)
	{
		loadx8((__m128i *)P + 8 * i, data);
		xorx8_1wise(data, mask, tmps);
		storex8((__m128i *)C + 8 * i, tmps);

		for (size_t j = 0; j < 4; j++)
		{
			mask[j] = mul16rev_128(pp1[0], pp2[0], mask[j]);
			mask[j] = mul16rev_128(pp1[0], pp2[0], mask[j]);
		}

		sum_x8(tmps, sum);
	}

	for (size_t i = 0; i < bx8rem; i++)
	{
		data[0] = _mm_loadu_si128((__m128i *)P + 8 * bx8len + i);
		tmps[0] = _mm_xor_si128(data[0], mask[0]);
		_mm_store_si128((__m128i *)C + 8 * bx8len + i, tmps[0]);
		mask[0] = mul2rev_128(ctx->poly_double128, mask[0]);
		sum = _mm_xor_si128(sum, tmps[0]);
	}

	return sum;
}

static inline void ex(eme_context* ctx, const uint8_t *P, size_t Plen, uint8_t *C)
{
	__m128i *L = L128_PTR(ctx);
	size_t blen = Plen / 16;
	size_t bx8len = blen / 8;
	size_t bx8rem = blen % 8;

	alignas(16) __m128i data[8];
	alignas(16) __m128i tmps[8];

	for (size_t i = 0; i < bx8len; i++)
	{
		loadx8((__m128i *)P + 8 * i, data);
		copyx8(data, tmps);
		aesx8(ctx->aesctx.keys128, tmps, tmps);
		xorx8_1wise(L + 8 * i, tmps, tmps);
		storex8((__m128i *)C + 8 * i, tmps);
	}

	for (size_t i = 0; i < bx8rem; i++)
	{
		data[0] = _mm_loadu_si128((__m128i *)P + 8 * bx8len + i);
		tmps[0] = aesenc128(data[0], ctx->aesctx.keys128);
		tmps[0] = _mm_xor_si128(L[8 * bx8len + i], tmps[0]);
		_mm_store_si128((__m128i *)C + 8 * bx8len + i, tmps[0]);
	}
}

static inline void eme(eme_context* ctx, uint8_t *T, const uint8_t *P, size_t Plen, uint8_t *C)
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
