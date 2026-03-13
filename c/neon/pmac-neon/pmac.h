#pragma once
#include "core.h"
#include "../common/graycode.h"

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
#define mul16rev(pp, X) byterev(mul16(pp, byterev(X)))

static inline __m128i half128(pmac_context ctx, __m128i x)
{
	alignas(16) __m128i y;
	alignas(16) __m128i c = _mm_srli_epi64(x, 63);
	alignas(16) __m128i m0 = _mm_setr_epi32(1, 0, 0, 0);
	alignas(16) __m128i m1 = _mm_setr_epi32(0, 0, 1, 0);
	alignas(16) __m128i c0 = _mm_and_si128(c, m0);
	c0 = _mm_bslli_si128(c0, 8);
	alignas(16) __m128i c1 = _mm_and_si128(c, m1);
	c1 = _mm_bsrli_si128(c1, 8);
	y = _mm_slli_epi64(x, 1);
	y = _mm_xor_si128(y, c0);
	alignas(16) __m128i z = ctx.invpoly;
	z = _mm_clmulepi64_si128(c1, z, 0);
	y = _mm_xor_si128(y, z);
	return y;
}

void pmacinit128(aes_context *aesctx, pmac_context *ctx, uint8_t *key)
{
	aesinit128(aesctx, key);
	ctx->L[0] = aesenc128(_mm_setzero_si128(), aesctx->keys128);
	ctx->Linv = half128(*ctx, ctx->L[0]);
	ctx->poly = _mm_setr_epi32(0x87, 0, 0, 0);
	ctx->pp = _mm_slli_si128(ctx->poly, 1);
	ctx->invpoly = _mm_setr_epi32(0x43, 0, 0, 0x80);

	for (size_t i = 1; i < 64; i++)
	{
		ctx->L[i] = mul2rev(ctx->pp, ctx->L[i - 1]);
	}
}

#define pmac_round(M, aesctx, ctx, offset, data, tmps, sum, i) \
	{                                                          \
		(data)[i] = _mm_loadu_si128(M + i);                    \
		(tmps)[i] = _mm_xor_si128((offset)[i], (data)[i]);     \
		(tmps)[i] = aesenc128((tmps)[i], aesctx.keys128);      \
		sum = _mm_xor_si128((tmps)[i], sum);                   \
	}

static inline __m128i pmac128x4(aes_context aesctx, pmac_context ctx, const uint8_t *M, size_t len)
{
	size_t brem = len % 16;
	size_t blen = (len + 16 - brem) / 16 - 1;
	size_t bx4len = blen / 4;
	size_t bx4rem = blen % 4;

	alignas(16) __m128i sum = _mm_setzero_si128();
	alignas(16) __m128i Li = ctx.L[0];
	alignas(16) __m128i offset[8];
	offset[0] = ctx.L[0];
	for (size_t i = 1; i < 4; i++)
	{
		offset[i] = _mm_xor_si128(offset[i - 1], ctx.L[_tzcnt_u64(i + 1)]);
	}

	alignas(16) __m128i data[8];
	alignas(16) __m128i tmps[8];

	for (size_t i = 0; i < bx4len; i++)
	{
		seq_graycode_x4(offset, ctx.L, ctx.L[1], Li, i);
		pmac_round((__m128i *)M + i * 4, aesctx, ctx, offset, data, tmps, sum, 0);
		pmac_round((__m128i *)M + i * 4, aesctx, ctx, offset, data, tmps, sum, 1);
		pmac_round((__m128i *)M + i * 4, aesctx, ctx, offset, data, tmps, sum, 2);
		pmac_round((__m128i *)M + i * 4, aesctx, ctx, offset, data, tmps, sum, 3);
	}

	for (size_t i = 0; i < bx4rem; i++)
	{
		seq_graycode_x1(offset, ctx.L, (bx4len * 4 + i));
		pmac_round((__m128i *)M + bx4len * 4 + i, aesctx, ctx, offset, data, tmps, sum, 0);
	}

	data[0] = ozp128(len - (blen) * 16, M + (blen) * 16);
	tmps[0] = _mm_xor_si128(sum, data[0]);

	if (brem > 0)
	{
		tmps[0] = _mm_xor_si128(tmps[0], ctx.Linv);
	}

	tmps[0] = aesenc128(tmps[0], aesctx.keys128);

	return tmps[0];
}

static inline __m128i pmac128x8(aes_context aesctx, pmac_context ctx, const uint8_t *M, size_t len)
{
	size_t brem = len % 16;
	size_t blen = (len + 16 - brem) / 16 - 1;
	size_t bx8len = blen / 8;
	size_t bx8rem = blen % 8;

	alignas(16) __m128i sum = _mm_setzero_si128();
	alignas(16) __m128i Li = ctx.L[0];
	alignas(16) __m128i offset[16];
	offset[0] = ctx.L[0];
	for (size_t i = 1; i < 8; i++)
	{
		offset[i] = _mm_xor_si128(offset[i - 1], ctx.L[_tzcnt_u64(i + 1)]);
	}

	alignas(16) __m128i data[16];
	alignas(16) __m128i tmps[16];

	for (size_t i = 0; i < bx8len; i++)
	{
		seq_graycode_x8(offset, ctx.L, ctx.L[1], Li, i);
		pmac_round((__m128i *)M + i * 4, aesctx, ctx, offset, data, tmps, sum, 0);
		pmac_round((__m128i *)M + i * 4, aesctx, ctx, offset, data, tmps, sum, 1);
		pmac_round((__m128i *)M + i * 4, aesctx, ctx, offset, data, tmps, sum, 2);
		pmac_round((__m128i *)M + i * 4, aesctx, ctx, offset, data, tmps, sum, 3);
		pmac_round((__m128i *)M + i * 4, aesctx, ctx, offset, data, tmps, sum, 4 + 0);
		pmac_round((__m128i *)M + i * 4, aesctx, ctx, offset, data, tmps, sum, 4 + 1);
		pmac_round((__m128i *)M + i * 4, aesctx, ctx, offset, data, tmps, sum, 4 + 2);
		pmac_round((__m128i *)M + i * 4, aesctx, ctx, offset, data, tmps, sum, 4 + 3);
	}

	for (size_t i = 0; i < bx8rem; i++)
	{
		seq_graycode_x1(offset, ctx.L, (bx8len * 8 + i));
		pmac_round((__m128i *)M + bx8len * 8 + i, aesctx, ctx, offset, data, tmps, sum, 0);
	}

	data[0] = ozp128(len - (blen) * 16, M + (blen) * 16);
	tmps[0] = _mm_xor_si128(sum, data[0]);

	if (brem > 0)
	{
		tmps[0] = _mm_xor_si128(tmps[0], ctx.Linv);
	}

	tmps[0] = aesenc128(tmps[0], aesctx.keys128);

	return tmps[0];
}