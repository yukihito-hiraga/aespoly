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
	alignas(16) uint8_t in[16];
	alignas(16) uint8_t out[16];
	_mm_store_si128((__m128i *)in, x);
	uint8_t msb = in[0] & 0x80;
	out[0] = (uint8_t)(in[0] >> 1);
	for (size_t i = 1; i < 16; i++)
	{
		out[i] = (uint8_t)((in[i] >> 1) | ((in[i - 1] & 1) << 7));
	}
	if (msb)
	{
		out[0] ^= 0x80;
		out[15] ^= 0x43;
	}
	return _mm_load_si128((__m128i *)out);
}

static inline __m128i double128_be(__m128i x)
{
	alignas(16) uint8_t in[16];
	alignas(16) uint8_t out[16];
	_mm_store_si128((__m128i *)in, x);
	uint8_t cond = (uint8_t)(in[15] & 0x80);
	uint8_t carry = 0;
	for (int i = 15; i >= 0; i--)
	{
		uint8_t next = (uint8_t)(in[i] >> 7);
		out[i] = (uint8_t)((in[i] << 1) | carry);
		carry = next;
	}
	if (cond)
	{
		out[15] ^= 0x87;
	}
	return _mm_load_si128((__m128i *)out);
}

void pmacinit128(aes_context *aesctx, pmac_context *ctx, uint8_t *key)
{
	aesinit128(aesctx, key);
	ctx->poly = _mm_setr_epi32(0x87, 0, 0, 0);
	ctx->pp = _mm_slli_si128(ctx->poly, 1);
	ctx->invpoly = _mm_setr_epi32(0x43, 0, 0, 0x80);
	ctx->L[0] = aesenc128(_mm_setzero_si128(), aesctx->keys128);
	ctx->Linv = half128(*ctx, ctx->L[0]);

	for (size_t i = 1; i < 64; i++)
	{
		ctx->L[i] = double128_be(ctx->L[i - 1]);
	}
}

#define pmac_round(M, aesctx, ctx, offset, data, tmps, sum, i) \
	{                                                          \
		(data)[i] = _mm_loadu_si128(M + i);                    \
		(tmps)[i] = _mm_xor_si128((offset)[i], (data)[i]);     \
		(tmps)[i] = aesenc128((tmps)[i], aesctx.keys128);      \
		sum = _mm_xor_si128((tmps)[i], sum);                   \
	}

static inline __m128i pmac128x4(aes_context *aesctx, const pmac_context *ctx, const uint8_t *M, size_t len)
{
	size_t m = (len + 15) / 16;
	if (m == 0)
	{
		m = 1;
	}
	size_t full_blocks = m - 1;
	alignas(16) __m128i sum = _mm_setzero_si128();
	alignas(16) __m128i offset[4];
	alignas(16) __m128i data[4];
	alignas(16) __m128i tmps[4];
	alignas(16) __m128i Li = ctx->L[2];
	size_t i = 0;
	if (full_blocks > 0)
	{
		offset[0] = ctx->L[0];
		for (size_t j = 1; j < 4; j++)
		{
			offset[j] = _mm_xor_si128(offset[j - 1], ctx->L[_tzcnt_u64(j + 1)]);
		}
	}
	for (; i + 4 <= full_blocks; i += 4)
	{
		pmac_round((__m128i *)(M + 16 * i), (*aesctx), (*ctx), offset, data, tmps, sum, 0);
		pmac_round((__m128i *)(M + 16 * i), (*aesctx), (*ctx), offset, data, tmps, sum, 1);
		pmac_round((__m128i *)(M + 16 * i), (*aesctx), (*ctx), offset, data, tmps, sum, 2);
		pmac_round((__m128i *)(M + 16 * i), (*aesctx), (*ctx), offset, data, tmps, sum, 3);
		if (i + 4 < full_blocks)
		{
			seq_graycode_x4(offset, ctx->L, ctx->L[1], Li, i + 1);
		}
	}
	if (i < full_blocks)
	{
		if (i == 0)
		{
			offset[0] = _mm_setzero_si128();
			seq_graycode_x1(offset, ctx->L, 1);
		}
	}
	for (; i < full_blocks; i++)
	{
		pmac_round((__m128i *)(M + 16 * i), (*aesctx), (*ctx), offset, data, tmps, sum, 0);
		if (i + 1 < full_blocks)
		{
			seq_graycode_x1(offset, ctx->L, i + 2);
		}
	}

	size_t tail_len = len - full_blocks * 16;
	__m128i S = _mm_xor_si128(sum, ozp128(tail_len, M + full_blocks * 16));
	if (tail_len == 16)
	{
		S = _mm_xor_si128(S, ctx->Linv);
	}
	return aesenc128(S, aesctx->keys128);
}

static inline __m128i pmac128x8(aes_context *aesctx, const pmac_context *ctx, const uint8_t *M, size_t len)
{
	size_t m = (len + 15) / 16;
	if (m == 0)
	{
		m = 1;
	}
	size_t full_blocks = m - 1;
	alignas(16) __m128i sum = _mm_setzero_si128();
	alignas(16) __m128i offset[8];
	alignas(16) __m128i data[8];
	alignas(16) __m128i tmps[8];
	alignas(16) __m128i Li = ctx->L[3];
	size_t i = 0;
	if (full_blocks > 0)
	{
		alignas(16) __m128i current = _mm_setzero_si128();
		for (size_t j = 0; j < 8; j++)
		{
			current = _mm_xor_si128(current, ctx->L[_tzcnt_u64(j + 1)]);
			offset[j] = current;
		}
	}
	for (; i + 8 <= full_blocks; i += 8)
	{
		pmac_round((__m128i *)(M + 16 * i), (*aesctx), (*ctx), offset, data, tmps, sum, 0);
		pmac_round((__m128i *)(M + 16 * i), (*aesctx), (*ctx), offset, data, tmps, sum, 1);
		pmac_round((__m128i *)(M + 16 * i), (*aesctx), (*ctx), offset, data, tmps, sum, 2);
		pmac_round((__m128i *)(M + 16 * i), (*aesctx), (*ctx), offset, data, tmps, sum, 3);
		pmac_round((__m128i *)(M + 16 * i), (*aesctx), (*ctx), offset, data, tmps, sum, 4);
		pmac_round((__m128i *)(M + 16 * i), (*aesctx), (*ctx), offset, data, tmps, sum, 5);
		pmac_round((__m128i *)(M + 16 * i), (*aesctx), (*ctx), offset, data, tmps, sum, 6);
		pmac_round((__m128i *)(M + 16 * i), (*aesctx), (*ctx), offset, data, tmps, sum, 7);
		if (i + 8 < full_blocks)
		{
			seq_graycode_x8(offset, ctx->L, ctx->L[2], Li, i / 8);
		}
	}
	if (i < full_blocks && i > 0)
	{
		offset[0] = offset[7];
		seq_graycode_x1(offset, ctx->L, i + 1);
	}
	for (; i < full_blocks; i++)
	{
		pmac_round((__m128i *)(M + 16 * i), (*aesctx), (*ctx), offset, data, tmps, sum, 0);
		if (i + 1 < full_blocks)
		{
			seq_graycode_x1(offset, ctx->L, i + 2);
		}
	}

	size_t tail_len = len - full_blocks * 16;
	__m128i S = _mm_xor_si128(sum, ozp128(tail_len, M + full_blocks * 16));
	if (tail_len == 16)
	{
		S = _mm_xor_si128(S, ctx->Linv);
	}
	return aesenc128(S, aesctx->keys128);
}
