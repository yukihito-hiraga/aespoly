#pragma once
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
#define mul16rev(pp, X) byterev(mul16(pp, byterev(X)))

static inline __m128i Lntz(ocb_context ctx, size_t i)
{
	alignas(16) __m128i x = ctx.L[0];
	uint64_t j = (size_t)i;
	j = _tzcnt_u64(j);
	x = byterev(x);
	for (size_t k = 0; k < j; k++)
	{
		x = mul2(ctx.pp, x);
	}
	return byterev(x);
}

void ocbinit128(ocb_context *ctx, uint8_t *key, bool omega)
{
	aesinit128(&ctx->aesctx, key);

	ctx->poly = _mm_setr_epi32(0x87, 0, 0, 0);
	ctx->pp = _mm_slli_si128(ctx->poly, 1);

	alignas(16) __m128i zero = _mm_setzero_si128();
	ctx->L_star = aesenc128(zero, ctx->aesctx.keys128);

	ctx->L_dollar = mul2rev(ctx->pp, ctx->L_star);

	ctx->L[0] = mul2rev(ctx->pp, ctx->L_dollar);

	for (size_t i = 1; i < 290; i++)
	{
		ctx->L[i] = mul2rev(ctx->pp, ctx->L[i - 1]);
	}
}

static inline __m128i ocbhash128(ocb_context ctx, const uint8_t *A, size_t len)
{
	size_t blen = len / 16;
	size_t brem = len % 16;

	alignas(16) __m128i sum = _mm_setzero_si128();
	alignas(16) __m128i offset = _mm_setzero_si128();
	alignas(16) __m128i data, tmp;

	for (size_t i = 0; i < blen; i++)
	{
		offset = _mm_xor_si128(ctx.L[_tzcnt_u64(i + 1)], offset);
		data = _mm_loadu_si128((__m128i *)A + i);
		tmp = _mm_xor_si128(offset, data);
		tmp = aesenc128(tmp, ctx.aesctx.keys128);
		sum = _mm_xor_si128(tmp, sum);
	}

	if (brem > 0)
	{
		offset = _mm_xor_si128(offset, ctx.L_star);
		data = ozp128(brem, A + len - brem);
		tmp = _mm_xor_si128(data, offset);
		tmp = aesenc128(tmp, ctx.aesctx.keys128);
		sum = _mm_xor_si128(tmp, sum);
	}

	return sum;
}

static inline __m128i ocbhash128x4(ocb_context ctx, size_t plen, const uint8_t *P)
{
	alignas(16) __m128i sum = _mm_setzero_si128();

	alignas(16) __m128i L0 = ctx.L[0];
	alignas(16) __m128i L1 = ctx.L[1];
	alignas(16) __m128i L2 = ctx.L[2];
	alignas(16) __m128i Li, tmp, offset, data;

	alignas(16) __m128i offset1 = _mm_setzero_si128();
	alignas(16) __m128i offset2 = offset1;
	alignas(16) __m128i offset3 = offset1;
	alignas(16) __m128i offset4 = offset1;

	alignas(16) __m128i data1, _data1;
	alignas(16) __m128i data2, _data2;
	alignas(16) __m128i data3, _data3;
	alignas(16) __m128i data4, _data4;

	size_t blen = plen / 16;
	size_t brem = plen % 16;

	size_t bx4len = blen / 4;
	size_t bx4rem = blen % 4;

	for (size_t i = 0; i < bx4len; i++)
	{
		Li = ctx.L[_tzcnt_u64(4 * i + 4)];

		offset1 = _mm_xor_si128(offset4, L0);
		offset2 = _mm_xor_si128(offset1, L1);
		offset3 = _mm_xor_si128(offset4, L1);
		offset4 = _mm_xor_si128(offset3, Li);

		_data1 = _mm_loadu_si128((__m128i *)P + 4 * i + 0);
		_data2 = _mm_loadu_si128((__m128i *)P + 4 * i + 1);
		_data3 = _mm_loadu_si128((__m128i *)P + 4 * i + 2);
		_data4 = _mm_loadu_si128((__m128i *)P + 4 * i + 3);

		data1 = _mm_xor_si128(_data1, offset1);
		data2 = _mm_xor_si128(_data2, offset2);
		data3 = _mm_xor_si128(_data3, offset3);
		data4 = _mm_xor_si128(_data4, offset4);

		data1 = aesenc128(data1, ctx.aesctx.keys128);
		data2 = aesenc128(data2, ctx.aesctx.keys128);
		data3 = aesenc128(data3, ctx.aesctx.keys128);
		data4 = aesenc128(data4, ctx.aesctx.keys128);

		sum = _mm_xor_si128(sum, data1);
		sum = _mm_xor_si128(sum, data2);
		sum = _mm_xor_si128(sum, data3);
		sum = _mm_xor_si128(sum, data4);
	}

	offset = offset4;

	for (size_t i = 0; i < bx4rem; i++)
	{
		Li = ctx.L[_tzcnt_u64(4 * bx4len + i + 1)];
		offset = _mm_xor_si128(offset, Li);

		_data1 = _mm_loadu_si128((__m128i *)P + bx4len * 4 + i);
		data1 = _mm_xor_si128(_data1, offset);
		data1 = aesenc128(data1, ctx.aesctx.keys128);
		sum = _mm_xor_si128(sum, data1);
	}

	if (brem > 0)
	{
		offset = _mm_xor_si128(offset, ctx.L_star);
		data = ozp128(brem, P + plen - brem);
		tmp = _mm_xor_si128(data, offset);
		tmp = aesenc128(tmp, ctx.aesctx.keys128);
		sum = _mm_xor_si128(tmp, sum);
	}

	return sum;
}

static inline __m128i gen_offset0(ocb_context ctx, size_t tlen, size_t nlen, const uint8_t *N)
{
	uint8_t nonce[16];
	memset(nonce, 0, 16);
	nonce[0] = nonce[0] ^ ((uint8_t)((tlen * 8) % 128)) << 1;
	nonce[15 - nlen] ^= 0x01;
	memcpy(nonce + 16 - nlen, N, nlen);
	uint8_t bottom = nonce[15] & 0x3f;
	nonce[15] &= 0xc0;
	alignas(16) __m128i Ktop = _mm_loadu_si128((__m128i *)nonce);

	Ktop = aesenc128(Ktop, ctx.aesctx.keys128);
	uint8_t stretch[24];
	_mm_storeu_si128((__m128i *)stretch, ((__m128i *)&Ktop)[0]);
	for (size_t i = 0; i < 8; i++)
	{
		stretch[16 + i] = stretch[0 + i] ^ stretch[1 + i];
	}
	size_t bbtm = bottom / 8;
	size_t rembtm = bottom % 8;
	for (size_t i = bbtm; i < bbtm + 16; i++)
	{
		stretch[i] <<= rembtm;
		for (size_t j = 0; j < rembtm; j++)
		{
			stretch[i] ^= ((stretch[i + 1] << j) & 0x80) >> (8 - rembtm + j);
		}
	}
	return _mm_loadu_si128((__m128i *)(stretch + bbtm));
}

static inline void ocbenc128x4_offline(ocb_context ctx, size_t tlen,
				       const uint8_t *N, size_t nlen,
				       const uint8_t *A, size_t alen,
				       const uint8_t *P, size_t plen,
				       uint8_t *C);

static inline void ocbenc128(ocb_context ctx, size_t tlen, const uint8_t *N, size_t nlen, const uint8_t *A, size_t alen, const uint8_t *P, size_t plen, uint8_t *C)
{

	alignas(16) __m128i checksum = _mm_setzero_si128();
	alignas(16) __m128i offset = gen_offset0(ctx, tlen, nlen, N);
	alignas(16) __m128i data, tmp;

	size_t blen = plen / 16;
	size_t brem = plen % 16;

	data = _mm_loadu_si128((__m128i *)P);

	for (size_t i = 0; i < blen; i++)
	{
		offset = _mm_xor_si128(offset, Lntz(ctx, i + 1));
		data = _mm_loadu_si128((__m128i *)P + i);
		tmp = _mm_xor_si128(data, offset);
		tmp = aesenc128(tmp, ctx.aesctx.keys128);
		tmp = _mm_xor_si128(tmp, offset);
		_mm_storeu_si128((__m128i *)C + i, tmp);

		checksum = _mm_xor_si128(checksum, data);
	}

	if (brem > 0)
	{
		offset = _mm_xor_si128(offset, ctx.L_star);
		data = ozp128(brem, P + plen - brem);
		tmp = aesenc128(offset, ctx.aesctx.keys128);
		tmp = _mm_xor_si128(data, tmp);
		memcpy(C + plen - brem, ((uint8_t *)&tmp), brem);
		checksum = _mm_xor_si128(checksum, data);
	}

	tmp = ctx.L_dollar;
	tmp = _mm_xor_si128(tmp, offset);
	tmp = _mm_xor_si128(checksum, tmp);

	alignas(16) __m128i tag = aesenc128(tmp, ctx.aesctx.keys128);

	tmp = ocbhash128(ctx, A, alen);
	tag = _mm_xor_si128(tag, tmp);

	uint8_t tagblk[16];
	_mm_storeu_si128((__m128i *)tagblk, tag);
	memcpy(C + plen, tagblk, tlen);
}

static inline void ocbenc128_offline(ocb_context ctx, size_t tlen, const uint8_t *N, size_t nlen, const uint8_t *A, size_t alen, const uint8_t *P, size_t plen, uint8_t *C)
{

	alignas(16) __m128i checksum = _mm_setzero_si128();
	alignas(16) __m128i offset = gen_offset0(ctx, tlen, nlen, N);
	alignas(16) __m128i data, tmp;

	size_t blen = plen / 16;
	size_t brem = plen % 16;

	data = _mm_loadu_si128((__m128i *)P);

	for (size_t i = 0; i < blen; i++)
	{
		offset = _mm_xor_si128(offset, ctx.L[_tzcnt_u64(i+1)]);

		data = _mm_loadu_si128((__m128i *)P + i);
		tmp = _mm_xor_si128(data, offset);
		tmp = aesenc128(tmp, ctx.aesctx.keys128);
		tmp = _mm_xor_si128(tmp, offset);
		_mm_storeu_si128((__m128i *)C + i, tmp);

		checksum = _mm_xor_si128(checksum, data);
	}

	if (brem > 0)
	{
		offset = _mm_xor_si128(offset, ctx.L_star);
		data = ozp128(brem, P + plen - brem);
		tmp = aesenc128(offset, ctx.aesctx.keys128);
		tmp = _mm_xor_si128(data, tmp);
		memcpy(C + plen - brem, ((uint8_t *)&tmp), brem);
		checksum = _mm_xor_si128(checksum, data);
	}

	tmp = ctx.L_dollar;
	tmp = _mm_xor_si128(tmp, offset);
	tmp = _mm_xor_si128(checksum, tmp);

	alignas(16) __m128i tag = aesenc128(tmp, ctx.aesctx.keys128);

	tmp = ocbhash128(ctx, A, alen);
	tag = _mm_xor_si128(tag, tmp);

	uint8_t tagblk[16];
	_mm_storeu_si128((__m128i *)tagblk, tag);
	memcpy(C + plen, tagblk, tlen);
}

static inline void ocbenc128x4(ocb_context ctx, size_t tlen, const uint8_t *N, size_t nlen, const uint8_t *A, size_t alen, const uint8_t *P, size_t plen, uint8_t *C)
{
	alignas(16) __m128i checksum = _mm_setzero_si128();
	alignas(16) __m128i Li, offset;
	alignas(16) __m128i offsets[4];
	alignas(16) __m128i data[4];
	alignas(16) __m128i tmps[4];

	offsets[0] = gen_offset0(ctx, tlen, nlen, N);
	offset = offsets[0];

	for (size_t i = 1; i < 4; i++)
	{
		offsets[i] = _mm_xor_si128(offsets[i - 1], ctx.L[_tzcnt_u64(i)]);
	}

	size_t blen = plen / 16;
	size_t brem = plen % 16;

	size_t bx4len = blen / 4;
	size_t bx4rem = blen % 4;

	for (size_t i = 0; i < bx4len; i++)
	{
		seq_graycode_x4_online(offsets, ctx, offset, i);

		loadx4((__m128i *)P + 4 * i, data);

		xorx4_1wise(data, offsets, tmps);

		aesx4(ctx.aesctx.keys128, tmps, tmps);

		xorx4_1wise(tmps, offsets, tmps);

		storex4((__m128i *)C + 4 * i, tmps);

		sum_x4(data, checksum);
	}

	for (size_t i = 0; i < bx4rem; i++)
	{
		offsets[0] = offset;
		seq_graycode_x1_online(offsets, ctx, 4 * bx4len + i);
		offset = offsets[0];
		data[0] = _mm_loadu_si128((__m128i *)P + bx4len * 4 + i);
		tmps[0] = _mm_xor_si128(data[0], offset);
		tmps[0] = aesenc128(tmps[0], ctx.aesctx.keys128);
		tmps[0] = _mm_xor_si128(tmps[0], offset);
		_mm_storeu_si128((__m128i *)C + bx4len * 4 + i, tmps[0]);

		checksum = _mm_xor_si128(checksum, data[0]);
	}

	if (brem > 0)
	{
		offset = _mm_xor_si128(offset, ctx.L_star);
		data[0] = ozp128(brem, P + plen - brem);
		tmps[0] = aesenc128(offset, ctx.aesctx.keys128);
		tmps[0] = _mm_xor_si128(data[0], tmps[0]);
		memcpy(C + plen - brem, ((uint8_t *)&tmps), brem);
		checksum = _mm_xor_si128(checksum, data[0]);
	}

	tmps[0] = _mm_xor_si128(ctx.L_dollar, offset);
	tmps[0] = _mm_xor_si128(checksum, tmps[0]);

	alignas(16) __m128i tag = aesenc128(tmps[0], ctx.aesctx.keys128);

	tmps[0] = ocbhash128(ctx, A, alen);
	tag = _mm_xor_si128(tag, tmps[0]);

	uint8_t tagblk[16];
	_mm_storeu_si128((__m128i *)tagblk, tag);
	memcpy(C + plen, tagblk, tlen);
}


static inline void ocbenc128x4_offline(ocb_context ctx, size_t tlen, const uint8_t *N, size_t nlen, const uint8_t *A, size_t alen, const uint8_t *P, size_t plen, uint8_t *C)
{
	alignas(16) __m128i checksum = _mm_setzero_si128();
	alignas(16) __m128i Li, offset;

	alignas(16) __m128i offsets[4];
	alignas(16) __m128i data[4];
	alignas(16) __m128i tmps[4];

	offsets[0] = gen_offset0(ctx, tlen, nlen, N);
	offset = offsets[0];

	for (size_t i = 1; i < 4; i++)
	{
		offsets[i] = _mm_xor_si128(offsets[i - 1], ctx.L[_tzcnt_u64(i)]);
	}

	size_t blen = plen / 16;
	size_t brem = plen % 16;

	size_t bx4len = blen / 4;
	size_t bx4rem = blen % 4;

	Li = ctx.L[3];

	for (size_t i = 0; i < bx4len; i++)
	{
		loadx4((__m128i *)P + 4 * i, data);
		seq_graycode_x4(offsets, ctx.L, ctx.L[1], Li, i);

		xorx4_1wise(data, offsets, tmps);

		aesx4(ctx.aesctx.keys128, tmps, tmps);

		xorx4_1wise(tmps, offsets, tmps);

		storex4((__m128i *)C + 4 * i, tmps);

		sum_x4(data, checksum);
	}

	for (size_t i = 0; i < bx4rem; i++)
	{
		offset = _mm_xor_si128(offset, ctx.L[_tzcnt_u64(4 * bx4len + i + 1)]);
		data[0] = _mm_loadu_si128((__m128i *)P + bx4len * 4 + i);
		tmps[0] = _mm_xor_si128(data[0], offset);
		tmps[0] = aesenc128(tmps[0], ctx.aesctx.keys128);
		tmps[0] = _mm_xor_si128(tmps[0], offset);
		_mm_storeu_si128((__m128i *)C + bx4len * 4 + i, tmps[0]);

		checksum = _mm_xor_si128(checksum, data[0]);
	}

	if (brem > 0)
	{
		offset = _mm_xor_si128(offset, ctx.L_star);
		data[0] = ozp128(brem, P + plen - brem);
		tmps[0] = aesenc128(offset, ctx.aesctx.keys128);
		tmps[0] = _mm_xor_si128(data[0], tmps[0]);
		memcpy(C + plen - brem, ((uint8_t *)&tmps), brem);
		checksum = _mm_xor_si128(checksum, data[0]);
	}

	tmps[0] = _mm_xor_si128(ctx.L_dollar, offset);
	tmps[0] = _mm_xor_si128(checksum, tmps[0]);

	alignas(16) __m128i tag = aesenc128(tmps[0], ctx.aesctx.keys128);

	tmps[0] = ocbhash128(ctx, A, alen);
	tag = _mm_xor_si128(tag, tmps[0]);

	uint8_t tagblk[16];
	_mm_storeu_si128((__m128i *)tagblk, tag);
	memcpy(C + plen, tagblk, tlen);
}

static inline void ocbenc128x8_offline(ocb_context ctx, size_t tlen, const uint8_t *N, size_t nlen, const uint8_t *A, size_t alen, const uint8_t *P, size_t plen, uint8_t *C)
{
	alignas(16) __m128i checksum = _mm_setzero_si128();
	alignas(16) __m128i Li, offset;

	alignas(16) __m128i offsets[8];
	alignas(16) __m128i data[8];
	alignas(16) __m128i tmps[8];

	offsets[0] = gen_offset0(ctx, tlen, nlen, N);
	offset = offsets[0];

	for (size_t i = 1; i < 8; i++)
	{
		offsets[i] = _mm_xor_si128(offsets[i - 1], ctx.L[_tzcnt_u64(i)]);
	}

	size_t blen = plen / 16;
	size_t brem = plen % 16;

	size_t bx8len = blen / 8;
	size_t bx8rem = blen % 8;

	Li = ctx.L[1];

	for (size_t i = 0; i < bx8len; i++)
	{
		loadx8((__m128i *)P + 8 * i, data);
			seq_graycode_x8(offsets, ctx.L, ctx.L[2], Li, i);

		xorx8_1wise(data, offsets, tmps);

		aesx8(ctx.aesctx.keys128, tmps, tmps);

		xorx8_1wise(tmps, offsets, tmps);

		storex8((__m128i *)C + 8 * i, tmps);

		sum_x8(data, checksum);
	}

	for (size_t i = 0; i < bx8rem; i++)
	{
		offset = _mm_xor_si128(offset, ctx.L[_tzcnt_u64(8 * bx8len + i + 1)]);
		data[0] = _mm_loadu_si128((__m128i *)P + bx8len * 8 + i);
		tmps[0] = _mm_xor_si128(data[0], offset);
		tmps[0] = aesenc128(tmps[0], ctx.aesctx.keys128);
		tmps[0] = _mm_xor_si128(tmps[0], offset);
		_mm_storeu_si128((__m128i *)C + bx8len * 8 + i, tmps[0]);

		checksum = _mm_xor_si128(checksum, data[0]);
	}

	if (brem > 0)
	{
		offset = _mm_xor_si128(offset, ctx.L_star);
		data[0] = ozp128(brem, P + plen - brem);
		tmps[0] = aesenc128(offset, ctx.aesctx.keys128);
		tmps[0] = _mm_xor_si128(data[0], tmps[0]);
		memcpy(C + plen - brem, ((uint8_t *)&tmps), brem);
		checksum = _mm_xor_si128(checksum, data[0]);
	}

	tmps[0] = _mm_xor_si128(ctx.L_dollar, offset);
	tmps[0] = _mm_xor_si128(checksum, tmps[0]);

	alignas(16) __m128i tag = aesenc128(tmps[0], ctx.aesctx.keys128);

	tmps[0] = ocbhash128(ctx, A, alen);
	tag = _mm_xor_si128(tag, tmps[0]);

	uint8_t tagblk[16];
	_mm_storeu_si128((__m128i *)tagblk, tag);
	memcpy(C + plen, tagblk, tlen);
}
