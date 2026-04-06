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

static inline void mul2_256(const __m128i *X, __m128i *Y)
{
	const __m128i onebits = _mm_set1_epi16(0x0101);
	const __m128i zero = _mm_setzero_si128();
	const __m128i red = _mm_setr_epi8(
		0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, (char)0x87);

	__m128i next0 = _mm_alignr_epi8(X[1], X[0], 1);
	__m128i next1 = _mm_alignr_epi8(zero, X[1], 1);

	__m128i carry0 = _mm_and_si128(_mm_srli_epi16(next0, 7), onebits);
	__m128i carry1 = _mm_and_si128(_mm_srli_epi16(next1, 7), onebits);

	Y[0] = _mm_xor_si128(_mm_add_epi8(X[0], X[0]), carry0);
	Y[1] = _mm_xor_si128(_mm_add_epi8(X[1], X[1]), carry1);

	if ((_mm_movemask_epi8(X[0]) & 1) != 0)
	{
		Y[1] = _mm_xor_si128(Y[1], red);
	}
}

static inline void mul16_256(const __m128i *X, __m128i *Y)
{
	static const uint8_t red_lo[16] = {
		0x00, 0x87, 0x0e, 0x89, 0x1c, 0x9b, 0x12, 0x95,
		0x38, 0xbf, 0x36, 0xb1, 0x24, 0xa3, 0x2a, 0xad
	};
	static const uint8_t red_hi[16] = {
		0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03,
		0x04, 0x04, 0x05, 0x05, 0x06, 0x06, 0x07, 0x07
	};

	const __m128i lo_mask = _mm_set1_epi8(0x0f);
	const __m128i hi_mask = _mm_set1_epi8((char)0xf0);
	const __m128i zero = _mm_setzero_si128();

	__m128i next0 = _mm_alignr_epi8(X[1], X[0], 1);
	__m128i next1 = _mm_alignr_epi8(zero, X[1], 1);

	__m128i carry0 = _mm_and_si128(_mm_srli_epi16(next0, 4), lo_mask);
	__m128i carry1 = _mm_and_si128(_mm_srli_epi16(next1, 4), lo_mask);
	__m128i shl0 = _mm_and_si128(_mm_slli_epi16(X[0], 4), hi_mask);
	__m128i shl1 = _mm_and_si128(_mm_slli_epi16(X[1], 4), hi_mask);

	Y[0] = _mm_xor_si128(shl0, carry0);
	Y[1] = _mm_xor_si128(shl1, carry1);

	uint32_t top = (uint32_t)_mm_cvtsi128_si32(X[0]);
	uint8_t nib = (uint8_t)(top >> 4);
	if (nib != 0)
	{
		__m128i red = _mm_setr_epi8(
			0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, (char)red_hi[nib], (char)red_lo[nib]);
		Y[1] = _mm_xor_si128(Y[1], red);
	}
}

#define simpiraenc(ctx, tmps)                     \
	{                                             \
		addkey256((ctx).key, tmps, tmps);         \
		simpira_b2x1_128((ctx).simpiractx, tmps); \
		addkey256((ctx).key, tmps, tmps);         \
	}

#define simpiraencx4(ctx, tmps)                   \
	{                                             \
		addkey256x4((ctx).key, tmps, tmps);       \
		simpira_b2x4_128((ctx).simpiractx, tmps); \
		addkey256x4((ctx).key, tmps, tmps);       \
	}

#define simpiraencx8(ctx, tmps)                   \
	{                                             \
		addkey256x8((ctx).key, tmps, tmps);       \
		simpira_b2x8_128((ctx).simpiractx, tmps); \
		addkey256x8((ctx).key, tmps, tmps);       \
	}

void emeinit(eme_context *ctx, uint8_t *key)
{
	ctx->poly[0] = _mm_setr_epi32(0x87, 0, 0, 0);
	ctx->poly[1] = _mm_setr_epi32(0x87, 0, 0, 0);

	ctx->pp[0] = _mm_slli_si128(ctx->poly[0], 1);
	ctx->pp[1] = _mm_slli_si128(ctx->poly[1], 1);

	uint8_t chunk[70];
	uint32_t x[2] = {0};

	for (size_t i = 0; i < 16; i++)
	{
		chunk[i] = x[0] & 0xff;
		chunk[i + 16] = (x[0] & 0xff00) >> 8;
		x[0] ^= 0x87;

		chunk[32 + i] = x[1] & 0xff;
		chunk[32 + i + 16] = (x[1] & 0xff00) >> 8;
		x[1] ^= 0x87;

		if (i % 2 == 1)
		{
			x[0] ^= 0x87 << 1;
			x[1] ^= 0x87 << 1;
		}
		if (i % 4 == 3)
		{
			x[0] ^= 0x87 << 2;
			x[1] ^= 0x87 << 2;
		}
		if (i % 8 == 7)
		{
			x[0] ^= 0x87 << 3;
			x[1] ^= 0x87 << 3;
		}
	}

	ctx->pp16_1[0] = _mm_load_si128((__m128i *)chunk);
	ctx->pp16_2[0] = _mm_load_si128((__m128i *)chunk + 1);
	ctx->pp16_1[1] = _mm_load_si128((__m128i *)chunk + 2);
	ctx->pp16_2[1] = _mm_load_si128((__m128i *)chunk + 3);

	loadx2((__m128i *)key, ctx->key);
	simpira_b2_init(&ctx->simpiractx);
	ctx->simpiractx.keys[0] = ctx->key[0];
	ctx->simpiractx.keys[1] = ctx->key[1];

	L[0] = _mm_setzero_si128();
	L[1] = _mm_setzero_si128();

	simpiraenc(*ctx, L);

	for (size_t i = 0; i < (Lsize / 2 - 2); i++)
	{
		mul2_256(L + 2 * i, L + 2 * i + 2);
	}
}

static inline void xe_x4(eme_context ctx, const uint8_t *P, size_t Plen, uint8_t *C, __m128i *sum)
{
	size_t blen = Plen / 32;
	size_t bx4len = blen / 4;
	size_t bx4rem = blen % 4;

	alignas(16) __m128i data[8];
	alignas(16) __m128i tmps[8];

	sum[0] = _mm_setzero_si128();
	sum[1] = _mm_setzero_si128();

	for (size_t i = 0; i < bx4len; i++)
	{
		loadx8((__m128i *)P + 8 * i, data);
		xorx8_1wise(L + 8 * i, data, tmps);
		simpiraencx4(ctx, tmps);
		storex8((__m128i *)C + 8 * i, tmps);
		sum_n2x4(tmps, sum);
	}

	for (size_t i = 0; i < bx4rem; i++)
	{
		loadx2((__m128i *)P + bx4len * 8 + 2 * i, data);
		xorx2_1wise(L + 8 * bx4len + 2 * i, data, tmps);
		simpiraenc(ctx, tmps);
		storex2((__m128i *)C + bx4len * 8 + 2 * i, tmps);
		sum[0] = _mm_xor_si128(sum[0], tmps[0]);
		sum[1] = _mm_xor_si128(sum[1], tmps[1]);
	}
}

static inline void xe_x8(eme_context ctx, const uint8_t *P, size_t Plen, uint8_t *C, __m128i *sum)
{
	size_t blen = Plen / 32;
	size_t bx8len = blen / 8;
	size_t bx8rem = blen % 8;

	alignas(16) __m128i data[16];
	alignas(16) __m128i tmps[16];

	sum[0] = _mm_setzero_si128();
	sum[1] = _mm_setzero_si128();

	for (size_t i = 0; i < bx8len; i++)
	{
		loadx16((__m128i *)P + 16 * i, data);
		xorx16_1wise(L + 16 * i, data, tmps);
		simpiraencx8(ctx, tmps);
		storex16((__m128i *)C + 16 * i, tmps);
		sum_n2x8(tmps, sum);
	}

	for (size_t i = 0; i < bx8rem; i++)
	{
		loadx2((__m128i *)P + bx8len * 16 + 2 * i, data);
		xorx2_1wise(L + 16 * bx8len + 2 * i, data, tmps);
		simpiraenc(ctx, tmps);
		storex2((__m128i *)C + bx8len * 16 + 2 * i, tmps);
		sum[0] = _mm_xor_si128(sum[0], tmps[0]);
		sum[1] = _mm_xor_si128(sum[1], tmps[1]);
	}
}

static inline void middle_x4(eme_context ctx, __m128i *M, const uint8_t *P, size_t Plen, uint8_t *C, __m128i *sum)
{
	size_t blen = Plen / 32;
	size_t bx4len = blen / 4;
	size_t bx4rem = blen % 4;

	alignas(16) __m128i data[8];
	alignas(16) __m128i tmps[8];
	alignas(16) __m128i mask[8];

	sum[0] = _mm_setzero_si128();
	sum[1] = _mm_setzero_si128();

	mul2_256(M, mask);
	mul2_256(mask, mask + 2);
	mul2_256(mask + 2, mask + 4);
	mul2_256(mask + 4, mask + 6);

	for (size_t i = 0; i < bx4len; i++)
	{
		loadx8((__m128i *)P + 8 * i, data);
		xorx8_1wise(data, mask, tmps);
		storex8((__m128i *)C + 8 * i, tmps);

		mul16_256(mask, mask);
		mul16_256(mask + 2, mask + 2);
		mul16_256(mask + 4, mask + 4);
		mul16_256(mask + 6, mask + 6);

		sum_n2x4(tmps, sum);
	}

	for (size_t i = 0; i < bx4rem; i++)
	{
		loadx2((__m128i *)P + bx4len * 8 + 2 * i, data);
		xorx2_1wise(data, mask, tmps);
		storex2((__m128i *)C + bx4len * 8 + 2 * i, tmps);
		mul2_256(mask, mask);
		sum[0] = _mm_xor_si128(sum[0], tmps[0]);
		sum[1] = _mm_xor_si128(sum[1], tmps[1]);
	}
}

static inline void middle_x8(eme_context ctx, __m128i* M, const uint8_t *P, size_t Plen, uint8_t *C, __m128i *sum)
{
	size_t blen = Plen / 32;
	size_t bx8len = blen / 8;
	size_t bx8rem = blen % 8;

	alignas(16) __m128i data[16];
	alignas(16) __m128i tmps[16];
	alignas(16) __m128i mask[16];

	sum[0] = _mm_setzero_si128();
	sum[1] = _mm_setzero_si128();

	mul2_256(M, mask);
	for (size_t i = 1; i < 8; i++)
	{
		mul2_256(mask + (i - 1) * 2, mask + i * 2);
	}

	for (size_t i = 0; i < bx8len; i++)
	{
		loadx16((__m128i *)P + 16 * i, data);
		xorx16_1wise(data, mask, tmps);
		storex16((__m128i *)C + 16 * i, tmps);

		mul16_256(mask + 0, mask + 0);
		mul16_256(mask + 2, mask + 2);
		mul16_256(mask + 4, mask + 4);
		mul16_256(mask + 6, mask + 6);
		mul16_256(mask + 8, mask + 8);
		mul16_256(mask + 10, mask + 10);
		mul16_256(mask + 12, mask + 12);
		mul16_256(mask + 14, mask + 14);


		sum_n2x8(tmps, sum);
	}

	for (size_t i = 0; i < bx8rem; i++)
	{
		loadx2((__m128i *)P + bx8len * 16 + 2 * i, data);
		xorx2_1wise(data, mask, tmps);
		storex2((__m128i *)C + bx8len * 16 + 2 * i, tmps);
		mul2_256(mask, mask);
		sum[0] = _mm_xor_si128(sum[0], tmps[0]);
		sum[1] = _mm_xor_si128(sum[1], tmps[1]);
	}
}

static inline void ex_x4(eme_context ctx, const uint8_t *P, size_t Plen, uint8_t *C)
{
	size_t blen = Plen / 32;
	size_t bx4len = blen / 4;
	size_t bx4rem = blen % 4;

	alignas(16) __m128i data[8];
	alignas(16) __m128i tmps[8];
	alignas(16) __m128i mask[8];

	for (size_t i = 0; i < bx4len; i++)
	{
		loadx8((__m128i *)P + 8 * i, data);
		copyx8(data, tmps);
		simpiraencx4(ctx, tmps);
		xorx8_1wise(L + 8 * i, tmps, tmps);
		storex8((__m128i *)C + 8 * i, tmps);
	}

	for (size_t i = 0; i < bx4rem; i++)
	{
		loadx2((__m128i *)P + bx4len * 8 + 2 * i, data);
		copyx2(data, tmps);
		simpiraenc(ctx, tmps);
		xorx2_1wise(L + 8 * bx4len + 2 * i, tmps, tmps);
		storex2((__m128i *)C + bx4len * 8 + 2 * i, tmps);
	}
}

static inline void ex_x8(eme_context ctx, const uint8_t *P, size_t Plen, uint8_t *C)
{
	size_t blen = Plen / 32;
	size_t bx8len = blen / 8;
	size_t bx8rem = blen % 8;

	alignas(16) __m128i data[16];
	alignas(16) __m128i tmps[16];
	alignas(16) __m128i mask[16];

	for (size_t i = 0; i < bx8len; i++)
	{
		loadx16((__m128i *)P + 16 * i, data);
		copyx16(data, tmps);
		simpiraencx8(ctx, tmps);
		xorx16_1wise(L + 16 * i, tmps, tmps);
		storex16((__m128i *)C + 16 * i, tmps);
	}

	for (size_t i = 0; i < bx8rem; i++)
	{
		loadx2((__m128i *)P + bx8len * 16 + 2 * i, data);
		copyx2(data, tmps);
		simpiraenc(ctx, tmps);
		xorx2_1wise(L + 16 * bx8len + 2 * i, tmps, tmps);
		storex2((__m128i *)C + bx8len * 16 + 2 * i, tmps);
	}
}

static inline void eme_x4(eme_context ctx, uint8_t *T, const uint8_t *P, size_t Plen, uint8_t *C)
{
	alignas(16) __m128i tmps[16];
	__m128i *t = tmps;
	__m128i *sp_ppp1 = tmps + 2;
	__m128i *mp = tmps + 4;
	__m128i *mc = tmps + 6;
	__m128i *sc = tmps + 8;
	__m128i *ccc1 = tmps + 10;
	__m128i *mpmc = tmps + 12;
	__m128i *sct = tmps + 14;

	loadx2((__m128i *)T, t);
	xe_x4(ctx, P, Plen, C, sp_ppp1);
	xorx2_1wise(sp_ppp1, t, mp);
	copyx2(mp, mc);
	simpiraenc(ctx, mc);
	xorx2_1wise(mp, mc, mpmc);
	middle_x4(ctx, mpmc, C + 32, Plen - 32, C + 32, sc);
	xorx2_1wise(sc, t, sct);
	xorx2_1wise(mc, sct, ccc1);
	storex2((__m128i *)C, ccc1);
	ex_x4(ctx, C, Plen, C);
}

static inline void eme_x8(eme_context ctx, uint8_t *T, const uint8_t *P, size_t Plen, uint8_t *C)
{
	alignas(16) __m128i tmps[16];
	__m128i *t = tmps;
	__m128i *sp_ppp1 = tmps + 2;
	__m128i *mp = tmps + 4;
	__m128i *mc = tmps + 6;
	__m128i *sc = tmps + 8;
	__m128i *ccc1 = tmps + 10;
	__m128i *mpmc = tmps + 12;
	__m128i *sct = tmps + 14;

	loadx2((__m128i *)T, t);
	xe_x8(ctx, P, Plen, C, sp_ppp1);
	xorx2_1wise(sp_ppp1, t, mp);
	copyx2(mp, mc);
	simpiraenc(ctx, mc);
	xorx2_1wise(mp, mc, mpmc);
	middle_x8(ctx, mpmc, C + 32, Plen - 32, C + 32, sc);
	xorx2_1wise(sc, t, sct);
	xorx2_1wise(mc, sct, ccc1);
	storex2((__m128i *)C, ccc1);
	ex_x8(ctx, C, Plen, C);
}
