#pragma once
#include "../common/graycode.h"
#include "core.h"

static inline __m128i mul2(__m128i pp, __m128i X)
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

static inline __m128i mul16(__m128i pp1, __m128i pp2, __m128i X)
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

#define mul2rev(pp, X) byterev(mul2(pp, byterev(X)))
#define mul16rev(pp1, pp2, X) byterev(mul16(pp1, pp2, byterev(X)))

void emeinit(eme_context *ctx, uint8_t *key)
{
	ctx->poly = _mm_setr_epi32(0x87, 0, 0, 0);
	ctx->pp = _mm_setr_epi32(0x87 << 8, 0, 0, 0);
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

	ctx->pp16_1 = _mm_load_si128((__m128i *)chunk);
	ctx->pp16_2 = _mm_load_si128((__m128i *)chunk + 1);

	aesinit128(&(ctx->aesctx), key);
	L[0] = aesenc128(_mm_setzero_si128(), ctx->aesctx.keys128);

	for (size_t i = 0; i < (Lsize - 1); i++)
	{
		L[i + 1] = mul2rev(ctx->pp, L[i]);
	}
}

char nn[40];

static inline __m128i xe_x4(eme_context ctx, const uint8_t *P, size_t Plen, uint8_t *C)
{
	size_t blen = Plen / 16;
	size_t bx4len = blen / 4;
	size_t bx4rem = blen % 4;

	alignas(16) __m128i data[4];
	alignas(16) __m128i tmps[4];

	alignas(16) __m128i sum = _mm_setzero_si128();

	for (size_t i = 0; i < bx4len; i++)
	{
		loadx4((__m128i *)P + 4 * i, data);
		xorx4_1wise(L + 4 * i, data, tmps);
		aesx4(ctx.aesctx.keys128, tmps, tmps);
		storex4((__m128i *)C + 4 * i, tmps);
		sum_x4(tmps, sum);
	}

	for (size_t i = 0; i < bx4rem; i++)
	{
		data[0] = _mm_loadu_si128((__m128i *)P + bx4len * 4 + i);
		tmps[0] = _mm_xor_si128(L[4 * bx4len + i], data[0]);
		tmps[0] = aesenc128(tmps[0], ctx.aesctx.keys128);
		_mm_store_si128((__m128i *)C + 4 * bx4len + i, tmps[0]);
		sum = _mm_xor_si128(sum, tmps[0]);
	}

	return sum;
}

static inline __m128i xe_x8(eme_context ctx, const uint8_t *P, size_t Plen, uint8_t *C)
{
	size_t blen = Plen / 16;
	size_t bx8len = blen / 8;
	size_t bx8rem = blen % 8;

	alignas(16) __m128i data[8];
	alignas(16) __m128i tmps[8];

	alignas(16) __m128i sum = _mm_setzero_si128();

	for (size_t i = 0; i < bx8len; i++)
	{
		loadx8((__m128i *)P + 8 * i, data);
		xorx8_1wise(L + 8 * i, data, tmps);
		aesx8(ctx.aesctx.keys128, tmps, tmps);
		storex8((__m128i *)C + 8 * i, tmps);
		sum_x8(tmps, sum);
	}

	for (size_t i = 0; i < bx8rem; i++)
	{
		data[0] = _mm_loadu_si128((__m128i *)P + bx8len * 8 + i);
		tmps[0] = _mm_xor_si128(L[8 * bx8len + i], data[0]);
		tmps[0] = aesenc128(tmps[0], ctx.aesctx.keys128);
		_mm_store_si128((__m128i *)C + 8 * bx8len + i, tmps[0]);
		sum = _mm_xor_si128(sum, tmps[0]);
	}

	return sum;
}

static inline __m128i middle_x4(eme_context ctx, __m128i M, const uint8_t *P, size_t Plen, uint8_t *C)
{
	size_t blen = Plen / 16;
	size_t bx4len = blen / 4;
	size_t bx4rem = blen % 4;

	alignas(16) __m128i data[4];
	alignas(16) __m128i tmps[4];
	alignas(16) __m128i mask[4];

	alignas(16) __m128i sum = _mm_setzero_si128();

	mask[0] = mul2rev(ctx.pp, M);
	mask[1] = mul2rev(ctx.pp, mask[0]);
	mask[2] = mul2rev(ctx.pp, mask[1]);
	mask[3] = mul2rev(ctx.pp, mask[2]);

	for (size_t i = 0; i < bx4len; i++)
	{
		loadx4((__m128i *)P + 4 * i, data);
		xorx4_1wise(data, mask, tmps);
		storex4((__m128i *)C + 4 * i, tmps);

		mask[0] = mul16rev(ctx.pp16_1, ctx.pp16_2, mask[0]);
		mask[1] = mul16rev(ctx.pp16_1, ctx.pp16_2, mask[1]);
		mask[2] = mul16rev(ctx.pp16_1, ctx.pp16_2, mask[2]);
		mask[3] = mul16rev(ctx.pp16_1, ctx.pp16_2, mask[3]);

		sum = _mm_xor_si128(tmps[0], sum);
		sum = _mm_xor_si128(tmps[1], sum);
		sum = _mm_xor_si128(tmps[2], sum);
		sum = _mm_xor_si128(tmps[3], sum);

	}

	for (size_t i = 0; i < bx4rem; i++)
	{
		data[0] = _mm_loadu_si128((__m128i *)P + 4 * bx4len + i);
		tmps[0] = _mm_xor_si128(data[0], mask[0]);
		_mm_store_si128((__m128i *)C + 4 * bx4len + i, tmps[0]);
		mask[0] = mul2rev(ctx.pp, mask[0]);
		sum = _mm_xor_si128(sum, tmps[0]);
	}

	return sum;
}

static inline __m128i middle_x8(eme_context ctx, __m128i M, const uint8_t *P, size_t Plen, uint8_t *C)
{
	size_t blen = Plen / 16;
	size_t bx8len = blen / 8;
	size_t bx8rem = blen % 8;

	alignas(16) __m128i data[8];
	alignas(16) __m128i tmps[8];
	alignas(16) __m128i mask[8];

	alignas(16) __m128i sum = _mm_setzero_si128();

	mask[0] = mul2rev(ctx.pp, M);

	for (size_t i = 1; i < 8; i++)
	{
		mask[i] = mul2rev(ctx.pp, mask[i-1]);
	}

	for (size_t i = 0; i < bx8len; i++)
	{
		loadx8((__m128i *)P + 8 * i, data);
		xorx8_1wise(data, mask, tmps);
		storex8((__m128i *)C + 8 * i, tmps);

		mask[0] = mul16rev(ctx.pp16_1, ctx.pp16_2, mask[0]);
		mask[0] = mul16rev(ctx.pp16_1, ctx.pp16_2, mask[0]);

		mask[1] = mul16rev(ctx.pp16_1, ctx.pp16_2, mask[1]);
		mask[1] = mul16rev(ctx.pp16_1, ctx.pp16_2, mask[1]);

		mask[2] = mul16rev(ctx.pp16_1, ctx.pp16_2, mask[2]);
		mask[2] = mul16rev(ctx.pp16_1, ctx.pp16_2, mask[2]);

		mask[3] = mul16rev(ctx.pp16_1, ctx.pp16_2, mask[3]);
		mask[3] = mul16rev(ctx.pp16_1, ctx.pp16_2, mask[3]);


		sum = _mm_xor_si128(tmps[0], sum);
		sum = _mm_xor_si128(tmps[1], sum);
		sum = _mm_xor_si128(tmps[2], sum);
		sum = _mm_xor_si128(tmps[3], sum);
		sum = _mm_xor_si128(tmps[4 + 0], sum);
		sum = _mm_xor_si128(tmps[4 + 1], sum);
		sum = _mm_xor_si128(tmps[4 + 2], sum);
		sum = _mm_xor_si128(tmps[4 + 3], sum);
	}

	for (size_t i = 0; i < bx8rem; i++)
	{
		data[0] = _mm_loadu_si128((__m128i *)P + 8 * bx8len + i);
		tmps[0] = _mm_xor_si128(data[0], mask[0]);
		_mm_store_si128((__m128i *)C + 8 * bx8len + i, tmps[0]);
		mask[0] = mul2rev(ctx.pp, mask[0]);
		sum = _mm_xor_si128(sum, tmps[0]);
	}

	return sum;
}

static inline void ex_x4(eme_context ctx, const uint8_t *P, size_t Plen, uint8_t *C)
{
	size_t blen = Plen / 16;
	size_t bx4len = blen / 4;
	size_t bx4rem = blen % 4;

	alignas(16) __m128i data[4];
	alignas(16) __m128i tmps[4];
	alignas(16) __m128i mask[4];

	for (size_t i = 0; i < bx4len; i++)
	{
		loadx4((__m128i *)P + 4 * i, data);
		copyx4(data, tmps);
		aesx4(ctx.aesctx.keys128, tmps, tmps);
		xorx4_1wise(L + 4 * i, tmps, tmps);
		storex4((__m128i *)C + 4 * i, tmps);
	}

	for (size_t i = 0; i < bx4rem; i++)
	{
		data[0] = _mm_loadu_si128((__m128i *)P + bx4len * 4 + i);
		tmps[0] = aesenc128(data[0], ctx.aesctx.keys128);
		tmps[0] = _mm_xor_si128(L[4 * bx4len + i], tmps[0]);
		_mm_store_si128((__m128i *)C + 4*bx4len + i, tmps[0]);
	}
}

static inline void ex_x8(eme_context ctx, const uint8_t *P, size_t Plen, uint8_t *C)
{
	size_t blen = Plen / 16;
	size_t bx8len = blen / 8;
	size_t bx8rem = blen % 8;

	alignas(16) __m128i data[8];
	alignas(16) __m128i tmps[8];
	alignas(16) __m128i mask[8];

	for (size_t i = 0; i < bx8len; i++)
	{
		loadx8((__m128i *)P + 8 * i, data);
		copyx8(data, tmps);
		aesx8(ctx.aesctx.keys128, tmps, tmps);
		xorx8_1wise(L + 8 * i, tmps, tmps);
		storex8((__m128i *)C + 8 * i, tmps);
	}

	for (size_t i = 0; i < bx8rem; i++)
	{
		data[0] = _mm_loadu_si128((__m128i *)P + bx8len * 8 + i);
		tmps[0] = aesenc128(data[0], ctx.aesctx.keys128);
		tmps[0] = _mm_xor_si128(L[8 * bx8len + i], tmps[0]);
		_mm_store_si128((__m128i *)C + 8*bx8len + i, tmps[0]);
	}
}

static inline void eme_x4(eme_context ctx, uint8_t *T, const uint8_t *P, size_t Plen, uint8_t *C)
{
	alignas(16) __m128i t, sp_ppp1, mp, mc, sc, ccc1;
	t = _mm_loadu_si128((__m128i *)T);
	sp_ppp1 = xe_x4(ctx, P, Plen, C);
	mp = _mm_xor_si128(sp_ppp1, t);
	mc = aesenc128(mp, ctx.aesctx.keys128);
	sc = middle_x4(ctx, _mm_xor_si128(mp, mc), C + 16, Plen - 16, C + 16);
	ccc1 = _mm_xor_si128(mc, _mm_xor_si128(sc, t));
	_mm_storeu_si128((__m128i *)C, ccc1);
	ex_x4(ctx, C, Plen, C);
}

static inline void eme_x8(eme_context ctx, uint8_t *T, const uint8_t *P, size_t Plen, uint8_t *C)
{
	alignas(16) __m128i t, sp_ppp1, mp, mc, sc, ccc1;
	t = _mm_loadu_si128((__m128i *)T);
	sp_ppp1 = xe_x8(ctx, P, Plen, C);
	mp = _mm_xor_si128(sp_ppp1, t);
	mc = aesenc128(mp, ctx.aesctx.keys128);
	sc = middle_x8(ctx, _mm_xor_si128(mp, mc), C + 16, Plen - 16, C + 16);
	ccc1 = _mm_xor_si128(mc, _mm_xor_si128(sc, t));
	_mm_storeu_si128((__m128i *)C, ccc1);
	ex_x8(ctx, C, Plen, C);
}
