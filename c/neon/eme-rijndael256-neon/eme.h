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

	init_rijndael256(&(ctx->rijndael256ctx), key);

	L[0] = _mm_setzero_si128();
	L[1] = _mm_setzero_si128();

	rijndael256_128(ctx->rijndael256ctx, L, L);

	for (size_t i = 0; i < (Lsize / 2 - 2); i++)
	{
		L[2 * i + 2] = mul2(ctx->pp[0], L[2 * i]);
		L[2 * i + 3] = mul2(ctx->pp[1], L[2 * i + 1]);
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
		rijndael256x4(ctx.rijndael256ctx, tmps, tmps);
		storex8((__m128i *)C + 8 * i, tmps);
		sum_n2x4(tmps, sum);
	}

	for (size_t i = 0; i < bx4rem; i++)
	{
		loadx2((__m128i *)P + bx4len * 8 + 2 * i, data);
		xorx2_1wise(L + 8 * bx4len + 2 * i, data, tmps);
		rijndael256_128(ctx.rijndael256ctx, tmps, tmps);
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
		rijndael256x8(ctx.rijndael256ctx, tmps, tmps);
		storex8((__m128i *)C + 16 * i, tmps);
		sum_n2x8(tmps, sum);
	}

	for (size_t i = 0; i < bx8rem; i++)
	{
		loadx2((__m128i *)P + bx8len * 16 + 2 * i, data);
		xorx2_1wise(L + 16 * bx8len + 2 * i, data, tmps);
		rijndael256_128(ctx.rijndael256ctx, tmps, tmps);
		storex2((__m128i *)C + bx8len * 16 + 2 * i, tmps);
		sum[0] = _mm_xor_si128(sum[0], tmps[0]);
		sum[1] = _mm_xor_si128(sum[1], tmps[1]);
	}
}

static inline void middle_x4(eme_context ctx, __m128i* M, const uint8_t *P, size_t Plen, uint8_t *C, __m128i *sum)
{
	size_t blen = Plen / 32;
	size_t bx4len = blen / 4;
	size_t bx4rem = blen % 4;

	alignas(16) __m128i data[8];
	alignas(16) __m128i tmps[8];
	alignas(16) __m128i mask[8];

	sum[0] = _mm_setzero_si128();
	sum[1] = _mm_setzero_si128();

	mask[0] = mul2(ctx.pp[0], M[0]);
	mask[2] = mul2(ctx.pp[0], mask[0]);
	mask[4] = mul2(ctx.pp[0], mask[2]);
	mask[6] = mul2(ctx.pp[0], mask[4]);

	mask[1] = mul2(ctx.pp[1], M[1]);
	mask[3] = mul2(ctx.pp[1], mask[1]);
	mask[5] = mul2(ctx.pp[1], mask[3]);
	mask[7] = mul2(ctx.pp[1], mask[5]);

	for (size_t i = 0; i < bx4len; i++)
	{
		loadx8((__m128i *)P + 8 * i, data);
		xorx8_1wise(data, mask, tmps);
		storex8((__m128i *)C + 8 * i, tmps);

		mask[0] = mul16(ctx.pp16_1[0], ctx.pp16_2[0], mask[0]);
		mask[2] = mul16(ctx.pp16_1[0], ctx.pp16_2[0], mask[2]);
		mask[4] = mul16(ctx.pp16_1[0], ctx.pp16_2[0], mask[4]);
		mask[6] = mul16(ctx.pp16_1[0], ctx.pp16_2[0], mask[6]);

		mask[1] = mul16(ctx.pp16_1[1], ctx.pp16_2[1], mask[1]);
		mask[3] = mul16(ctx.pp16_1[1], ctx.pp16_2[1], mask[3]);
		mask[5] = mul16(ctx.pp16_1[1], ctx.pp16_2[1], mask[5]);
		mask[7] = mul16(ctx.pp16_1[1], ctx.pp16_2[1], mask[7]);

		sum_n2x4(tmps, sum);
	}

	for (size_t i = 0; i < bx4rem; i++)
	{
		loadx2((__m128i *)P + bx4len * 8 + 2 * i, data);
		xorx2_1wise(data, mask, tmps);
		storex2((__m128i *)C + bx4len * 8 + 2 * i, tmps);
		mask[0] = mul2(ctx.pp[0], mask[0]);
		mask[1] = mul2(ctx.pp[1], mask[1]);
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

	mask[0] = mul2(ctx.pp[0], M[0]);
	mask[1] = mul2(ctx.pp[1], M[1]);
	for (size_t i = 1; i < 8; i++)
	{
		mask[2*i] = mul2(ctx.pp[0], mask[(i-1)*2]);	
		mask[2*i+1] = mul2(ctx.pp[1], mask[(i-1)*2+1]);
	}

	for (size_t i = 0; i < bx8len; i++)
	{
		loadx16((__m128i *)P + 16 * i, data);
		xorx16_1wise(data, mask, tmps);
		storex16((__m128i *)C + 16 * i, tmps);

		mask[0] = mul16(ctx.pp16_1[0], ctx.pp16_2[0], mask[0]);
		mask[0] = mul16(ctx.pp16_1[0], ctx.pp16_2[0], mask[0]);

		mask[2] = mul16(ctx.pp16_1[0], ctx.pp16_2[0], mask[2]);
		mask[2] = mul16(ctx.pp16_1[0], ctx.pp16_2[0], mask[2]);

		mask[4] = mul16(ctx.pp16_1[0], ctx.pp16_2[0], mask[4]);
		mask[4] = mul16(ctx.pp16_1[0], ctx.pp16_2[0], mask[4]);

		mask[6] = mul16(ctx.pp16_1[0], ctx.pp16_2[0], mask[6]);
		mask[6] = mul16(ctx.pp16_1[0], ctx.pp16_2[0], mask[6]);


		mask[1] = mul16(ctx.pp16_1[1], ctx.pp16_2[1], mask[1]);
		mask[1] = mul16(ctx.pp16_1[1], ctx.pp16_2[1], mask[1]);

		mask[3] = mul16(ctx.pp16_1[1], ctx.pp16_2[1], mask[3]);
		mask[3] = mul16(ctx.pp16_1[1], ctx.pp16_2[1], mask[3]);

		mask[5] = mul16(ctx.pp16_1[1], ctx.pp16_2[1], mask[5]);
		mask[5] = mul16(ctx.pp16_1[1], ctx.pp16_2[1], mask[5]);

		mask[7] = mul16(ctx.pp16_1[1], ctx.pp16_2[1], mask[7]);
		mask[7] = mul16(ctx.pp16_1[1], ctx.pp16_2[1], mask[7]);


		sum_n2x8(tmps, sum);
	}

	for (size_t i = 0; i < bx8rem; i++)
	{
		loadx2((__m128i *)P + bx8len * 16 + 2 * i, data);
		xorx2_1wise(data, mask, tmps);
		storex2((__m128i *)C + bx8len * 16 + 2 * i, tmps);
		mask[0] = mul2(ctx.pp[0], mask[0]);
		mask[1] = mul2(ctx.pp[1], mask[1]);
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
		rijndael256x4(ctx.rijndael256ctx, data, tmps);
		xorx8_1wise(L + 8 * i, tmps, tmps);
		storex8((__m128i *)C + 8 * i, tmps);
	}

	for (size_t i = 0; i < bx4rem; i++)
	{
		loadx2((__m128i *)P + bx4len * 8 + 2 * i, data);
		rijndael256_128(ctx.rijndael256ctx, data, tmps);
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
		rijndael256x8(ctx.rijndael256ctx, data, tmps);
		xorx16_1wise(L + 16 * i, tmps, tmps);
		storex16((__m128i *)C + 16 * i, tmps);
	}

	for (size_t i = 0; i < bx8rem; i++)
	{
		loadx2((__m128i *)P + bx8len * 16 + 2 * i, data);
		rijndael256_128(ctx.rijndael256ctx, data, tmps);
		xorx2_1wise(L + 16 * bx8len + 2 * i, tmps, tmps);
		storex2((__m128i *)C + bx8len * 16 + 2 * i, tmps);
	}
}

static inline void eme_x4(eme_context ctx, uint8_t *T, const uint8_t *P, size_t Plen, uint8_t *C)
{
	alignas(16) __m128i tmps[20];
	__m128i *t = tmps;
	__m128i *sp_ppp1 = tmps + 2;
	__m128i *mp = tmps+4;
	__m128i *mc = tmps + 6;
	__m128i *sc = tmps + 8;
	__m128i* ccc1 = tmps + 10;
	__m128i *mpmc = tmps + 12;
	__m128i *sct = tmps + 14;

	loadx2((__m128i *)T, t);

	xe_x4(ctx, P, Plen, C, sp_ppp1);
	xorx2_1wise(sp_ppp1, t, mp);
	rijndael256_128(ctx.rijndael256ctx, mp, mc);
	xorx2_1wise(mp, mc, mpmc);
	middle_x4(ctx, mpmc, C + 32, Plen - 32, C + 32, sc);
	xorx2_1wise(sc, t, sct);
	xorx2_1wise(mc, sct, ccc1);
	storex2((__m128i *)C, ccc1);
	ex_x4(ctx, C, Plen, C);
}

static inline void eme_x8(eme_context ctx, uint8_t *T, const uint8_t *P, size_t Plen, uint8_t *C)
{
	alignas(16) __m128i tmps[24];
	__m128i *t = tmps;
	__m128i *sp_ppp1 = tmps + 2;
	__m128i *mp = tmps+4;
	__m128i *mc = tmps + 6;
	__m128i *sc = tmps + 8;
	__m128i* ccc1 = tmps + 10;
	__m128i *mpmc = tmps + 12;
	__m128i *sct = tmps + 14;

	loadx2((__m128i *)T, t);

	xe_x8(ctx, P, Plen, C, sp_ppp1);
	xorx2_1wise(sp_ppp1, t, mp);
	rijndael256_128(ctx.rijndael256ctx, mp, mc);
	xorx2_1wise(mp, mc, mpmc);
	middle_x8(ctx, mpmc, C + 32, Plen - 32, C + 32, sc);
	xorx2_1wise(sc, t, sct);
	xorx2_1wise(mc, sct, ccc1);
	storex2((__m128i *)C, ccc1);
	ex_x8(ctx, C, Plen, C);
}
