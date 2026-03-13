#pragma once
#include "core.h"

static inline __m512i double512(ocb_context ctx, __m512i x)
{
	alignas(512) __m512i y;
	alignas(512) __m512i c = _mm512_srli_epi64(x, 63);
	alignas(512) __m512i m0 = _mm512_setr_epi64(1, 0, 1, 0, 1, 0, 1, 0);
	alignas(512) __m512i m1 = _mm512_setr_epi64(0, 1, 0, 1, 0, 1, 0, 1);
	alignas(512) __m512i c0 = _mm512_and_si512(c, m0);
	c0 = _mm512_bslli_epi128(c0, 8);
	alignas(512) __m512i c1 = _mm512_and_si512(c, m1);
	c1 = _mm512_bsrli_epi128(c1, 8);
	y = _mm512_slli_epi64(x, 1);
	y = _mm512_xor_si512(y, c0);
	alignas(512) __m512i z = ctx.poly;
	z = _mm512_mullo_epi64(c1, z);
	y = _mm512_xor_si512(y, z);
	return y;
}

static inline __m512i double512_bymul(ocb_context ctx, __m512i x)
{
	alignas(512) __m512i y = ctx.two;

	alignas(512) __m512i pp00 = _mm512_clmulepi64_epi128(x, y, 0x00);
	alignas(512) __m512i pp10 = _mm512_clmulepi64_epi128(x, y, 0x01);

	alignas(512) __m512i ppmid_ls = _mm512_bslli_epi128(pp10, 8);
	alignas(512) __m512i carry = _mm512_bsrli_epi128(pp10, 8);

	alignas(512) __m512i modulo = ctx.poly;

	// maybe faster than clmul_epi128
	modulo = _mm512_mullo_epi64(modulo, carry);

	alignas(512) __m512i carryless = _mm512_xor_si512(ppmid_ls, pp00);

	y = _mm512_xor_si512(carryless, modulo);

	return y;
}

static inline __m512i double512_refl(ocb_context ctx, __m512i _x)
{
	return _mm512_shuffle_epi8(double512_bymul(ctx, _mm512_shuffle_epi8(_x, ctx.shuffle)), ctx.shuffle);
}

// intended for only L2 * 2^i
static inline __m512i gmul512(ocb_context ctx, __m512i x, __m512i y)
{
	/*
	alignas(512) __m512i tmp0 = _mm512_clmulepi64_epi128(x, y, 0x00);
	alignas(512) __m512i tmp2 = _mm512_clmulepi64_epi128(x, y, 0x01);
	alignas(512) __m512i tmp3 = _mm512_clmulepi64_epi128(x, y, 0x10);
	alignas(512) __m512i tmp1 = _mm512_clmulepi64_epi128(x, y, 0x11);

	tmp2 = _mm512_xor_si512(tmp2, tmp3);
	tmp3 = _mm512_bsrli_epi128(tmp2, 8);
	tmp2 = _mm512_bslli_epi128(tmp2, 8);
	tmp2 = _mm512_xor_si512(tmp2, tmp0);
	tmp1 = _mm512_xor_si512(tmp3, tmp1);

	alignas(512) __m512i poly = ctx.poly;

	tmp0 = _mm512_clmulepi64_epi128(tmp1, poly, 0x00);
	tmp1 = _mm512_clmulepi64_epi128(tmp1, poly, 0x01);

	tmp3 = _mm512_bslli_epi128(tmp1, 8);
	tmp3 = _mm512_xor_si512(tmp0, tmp3);
	tmp1 = _mm512_bsrli_epi128(tmp1, 8);

	tmp0 = _mm512_clmulepi64_epi128(tmp1, poly, 0x00);

	alignas(512) __m512i z = _mm512_xor_si512(tmp3, tmp2);
	z = _mm512_xor_si512(z, tmp0);
	return z;
	*/

	alignas(512) __m512i tmp0 = _mm512_clmulepi64_epi128(x, y, 0x00);
	alignas(512) __m512i tmp1 = _mm512_clmulepi64_epi128(x, y, 0x01);

	alignas(512) __m512i tmp2 = _mm512_bslli_epi128(tmp1, 8);
	tmp1 = _mm512_bsrli_epi128(tmp1, 8);
	tmp2 = _mm512_xor_si512(tmp2, tmp0);

	alignas(512) __m512i poly = ctx.poly;

	tmp0 = _mm512_clmulepi64_epi128(tmp1, poly, 0x00);
	return _mm512_xor_si512(tmp0, tmp2);
}

static inline __m512i quadruple512_bymul(ocb_context ctx, __m512i x)
{
	alignas(512) __m512i y = ctx.four;
	alignas(512) __m512i pp00 = _mm512_clmulepi64_epi128(x, y, 0x00);
	alignas(512) __m512i pp01 = _mm512_clmulepi64_epi128(x, y, 0x01);

	alignas(512) __m512i ppmid = pp01;
	alignas(512) __m512i ppmid_u = _mm512_bsrli_epi128(ppmid, 8);
	alignas(512) __m512i ppmid_l = _mm512_bslli_epi128(ppmid, 8);
	alignas(512) __m512i lower1 = _mm512_xor_si512(ppmid_l, pp00);
	alignas(512) __m512i upper1 = ppmid_u;

	alignas(512) __m512i poly = ctx.poly;

	pp00 = _mm512_clmulepi64_epi128(upper1, poly, 0x00);

	alignas(512) __m512i z = _mm512_xor_si512(lower1, pp00);
	return z;
}

static inline __m512i quadruple512_refl(ocb_context ctx, __m512i x)
{
	return _mm512_shuffle_epi8(quadruple512_bymul(ctx, _mm512_shuffle_epi8(x, ctx.shuffle)), ctx.shuffle);
}

// note: result stored in the last 128bit-block
static inline __m512i Lntz(ocb_context ctx, size_t i)
{
	alignas(512) __m512i x = _mm512_setzero_si512();
	uint64_t j = (size_t)i;
	j = _tzcnt_u64(j);
	((__m128 *)&x)[3] = ((__m128 *)&ctx.Ltbl[0])[0];
	x = _mm512_shuffle_epi8(x, ctx.shuffle);
	for (size_t k = 0; k < j; k++)
	{
		x = double512_bymul(ctx, x);
	}
	return _mm512_shuffle_epi8(x, ctx.shuffle);
}

inline __m512i omegax2(ocb_context ctx, __m512i omega)
{
	alignas(64) __m512i tmp1, tmp2;
	omega = _mm512_slli_epi32(omega, 1);
	tmp1 = _mm512_srli_epi32(omega, 31);
	tmp1 = _mm512_shuffle_epi32(tmp1, _MM_SHUFFLE(2, 1, 0, 3));
	tmp2 = _mm512_and_si512(ctx.mask, tmp1);
	tmp1 = _mm512_xor_si512(tmp1, tmp2);

	tmp2 = _mm512_shuffle_epi8(ctx.poly, tmp2);
	omega = _mm512_xor_si512(omega, tmp1);
	omega = _mm512_xor_si512(omega, tmp2);
	return omega;
}

inline __m512i omegax16(ocb_context ctx, __m512i omega)
{
	alignas(64) __m512i tmp1, tmp2;
	omega = _mm512_slli_epi32(omega, 4);
	tmp1 = _mm512_srli_epi32(omega, 28);
	tmp1 = _mm512_shuffle_epi32(tmp1, _MM_SHUFFLE(2, 1, 0, 3));
	tmp2 = _mm512_and_si512(ctx.mask, tmp1);
	tmp1 = _mm512_xor_si512(tmp1, tmp2);

	tmp2 = _mm512_shuffle_epi8(ctx.poly, tmp2);
	omega = _mm512_xor_si512(omega, tmp1);
	omega = _mm512_xor_si512(omega, tmp2);
	return omega;
}

inline __m512i omegax65535(ocb_context ctx, __m512i omega)
{
	alignas(64) __m512i tmp1, tmp2;
	tmp2 = _mm512_bslli_epi128(omega, 2);

	tmp1 = _mm512_bsrli_epi128(omega, 14);

	tmp1 = _mm512_shuffle_epi8(ctx.poly, tmp1);
	return _mm512_xor_si512(tmp2, tmp1);
}

void ocbinit512(aes_context *aesctx, ocb_context *ctx, uint8_t *key)
{
	aesctx->keys[0] = _mm512_broadcast_i64x2(_mm_loadu_si128((__m128i *)key));
	aesctx->keys[1] = aeskeyex(aesctx->keys[0], 0x01);
	aesctx->keys[2] = aeskeyex(aesctx->keys[1], 0x02);
	aesctx->keys[3] = aeskeyex(aesctx->keys[2], 0x04);
	aesctx->keys[4] = aeskeyex(aesctx->keys[3], 0x08);
	aesctx->keys[5] = aeskeyex(aesctx->keys[4], 0x10);
	aesctx->keys[6] = aeskeyex(aesctx->keys[5], 0x20);
	aesctx->keys[7] = aeskeyex(aesctx->keys[6], 0x40);
	aesctx->keys[8] = aeskeyex(aesctx->keys[7], 0x80);
	aesctx->keys[9] = aeskeyex(aesctx->keys[8], 0x1B);
	aesctx->keys[10] = aeskeyex(aesctx->keys[9], 0x36);

	aesctx->keys128[0] = _mm_loadu_si128((__m128i *)key);
	aesctx->keys128[1] = aeskeyex128(aesctx->keys128[0], 0x01);
	aesctx->keys128[2] = aeskeyex128(aesctx->keys128[1], 0x02);
	aesctx->keys128[3] = aeskeyex128(aesctx->keys128[2], 0x04);
	aesctx->keys128[4] = aeskeyex128(aesctx->keys128[3], 0x08);
	aesctx->keys128[5] = aeskeyex128(aesctx->keys128[4], 0x10);
	aesctx->keys128[6] = aeskeyex128(aesctx->keys128[5], 0x20);
	aesctx->keys128[7] = aeskeyex128(aesctx->keys128[6], 0x40);
	aesctx->keys128[8] = aeskeyex128(aesctx->keys128[7], 0x80);
	aesctx->keys128[9] = aeskeyex128(aesctx->keys128[8], 0x1B);
	aesctx->keys128[10] = aeskeyex128(aesctx->keys128[9], 0x36);

	ctx->poly = _mm512_setr_epi64(0x87, 0, 0x87, 0, 0x87, 0, 0x87, 0);
	ctx->blocklength = 16;

	ctx->shuffle = _mm512_broadcast_i64x2(_mm_setr_epi8(0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00));

	alignas(512) __m512i zero = _mm512_setzero_si512();
	ctx->two = _mm512_setr_epi64(2, 0, 2, 0, 2, 0, 2, 0);
	ctx->four = _mm512_setr_epi64(4, 0, 4, 0, 4, 0, 4, 0);
	ctx->mask = _mm512_set4_epi32(0, 0, 0, 0b00001111);
	ctx->L_star = aesenc512(zero, aesctx->keys);

	alignas(64) __m512i tmp = _mm512_setr_epi64(1, 0, 2, 0, 3, 0, 4, 0);
	{

		alignas(512) __m512i pp00 = _mm512_clmulepi64_epi128(ctx->poly, tmp, 0x00);
		alignas(512) __m512i pp11 = _mm512_clmulepi64_epi128(ctx->poly, tmp, 0x11);
		alignas(512) __m512i pp10 = _mm512_clmulepi64_epi128(ctx->poly, tmp, 0x01);
		alignas(512) __m512i pp01 = _mm512_clmulepi64_epi128(ctx->poly, tmp, 0x10);

		alignas(512) __m512i ppmid = _mm512_xor_si512(pp10, pp01);
		alignas(512) __m512i ppmid_ls = _mm512_bslli_epi128(ppmid, 8);
		alignas(512) __m512i ppmid_rs = _mm512_bsrli_epi128(ppmid, 8);
		alignas(512) __m512i ppupper = _mm512_xor_si512(ppmid_rs, pp11);
		alignas(512) __m512i pplower = _mm512_xor_si512(ppmid_ls, pp00);

		alignas(512) __m512i x0 = _mm512_clmulepi64_epi128(pplower, ctx->poly, 0x10);
		alignas(512) __m512i y0 = _mm512_shuffle_epi32(pplower, 78);
		alignas(512) __m512i y1 = _mm512_xor_si512(y0, x0);
		alignas(512) __m512i x1 = _mm512_clmulepi64_epi128(y1, ctx->poly, 0x10);
		alignas(512) __m512i y2 = _mm512_shuffle_epi32(y1, 78);
		alignas(512) __m512i ppl_reduced = _mm512_xor_si512(y2, x1);

		ctx->omegafirst = _mm512_xor_si512(ppupper, ppl_reduced);
	}

	ctx->L_dollar = double512_refl(*ctx, ctx->L_star);

	Lstbl[0] = double512_refl(*ctx, ctx->L_dollar);
	Lstbl_o[0] = Lstbl[0];
	Lstbl2[0] = _mm512_setzero_si512();

	((__m128i *)&Lstbl2[0])[3] = ((__m128i *)&Lstbl[0])[3];

	for (size_t i = 1; i < max_num_block; i++)
	{
		Lstbl_o[i] = double512_refl(*ctx, Lstbl_o[i - 1]);
		Lstbl[i] = quadruple512_refl(*ctx, Lstbl[i - 1]);
		Lstbl2[i] = _mm512_setzero_si512();
		((__m128i *)&Lstbl2[0])[3] = ((__m128i *)&Lstbl[0])[3];
	}

	ctx->Ltbl_first = _mm512_setzero_si512();

	((__m128i *)&ctx->Ltbl_first)[0] = _mm_xor_si128(((__m128i *)&ctx->Ltbl[0])[0], ((__m128i *)&ctx->Ltbl[1])[0]);
	((__m128i *)&ctx->Ltbl_first)[1] = ((__m128i *)&ctx->Ltbl[0])[0];
	((__m128i *)&ctx->Ltbl_first)[2] = ((__m128i *)&ctx->Ltbl[1])[0];

	tmp = _mm512_xor_si512(Lstbl[__tzcnt_u64(0)], Lstbl2[__tzcnt_u64(0)]);

	ctx->Ltbl_first = _mm512_xor_si512(ctx->Ltbl_first, tmp);
}

static inline __m512i process_offset(__m512i offset, size_t i)
{
	alignas(64) __m512i L1, Li1, Li2;
	Li1 = Lstbl[_tzcnt_u64(4 * i)];
	Li2 = _mm512_xor_si512(Lstbl2[_tzcnt_u64(4 * i)], Lstbl2[_tzcnt_u64(4 * i + 4)]);
	offset = _mm512_xor_si512(offset, L1);
	offset = _mm512_xor_si512(offset, Li1);
	offset = _mm512_xor_si512(offset, Li2);
	return offset;
}

static inline __m128i ocbhash512(aes_context aesctx, ocb_context ctx, const uint8_t *A, size_t len)
{
	size_t blen = len / ctx.blocklength;
	size_t b512len = blen / 4;
	size_t brem = len % ctx.blocklength;
	size_t b512rem = blen % 4;

	alignas(64) __m128i sum = _mm_setzero_si128();

	alignas(64) __m512i sum512 = _mm512_setzero_si512();
	alignas(64) __m512i offset = _mm512_setzero_si512();
	alignas(64) __m512i data, tmp;

	alignas(64) __m512i L1, Li1, Li2;

	for (size_t i = 0; i < b512len; i++)
	{
		Li1 = Lstbl[_tzcnt_u64(4 * i)];
		Li2 = _mm512_xor_si512(Lstbl2[_tzcnt_u64(4 * i)], Lstbl2[_tzcnt_u64(4 * i + 4)]);
		offset = _mm512_xor_si512(offset, L1);
		offset = _mm512_xor_si512(offset, Li1);
		offset = _mm512_xor_si512(offset, Li2);

		data = _mm512_loadu_si512((__m512i *)A + i);
		tmp = _mm512_xor_si512(offset, data);
		tmp = aesenc512(tmp, aesctx.keys);
		sum512 = _mm512_xor_si512(tmp, sum512);
	}

	tmp = _mm512_setzero_si512();
	((__m128i *)&tmp)[0] = ((__m128i *)&sum512)[2];
	((__m128i *)&tmp)[1] = ((__m128i *)&sum512)[3];
	tmp = _mm512_xor_si512(sum512, tmp);
	sum = _mm_xor_si128(((__m128i *)&tmp)[0], ((__m128i *)&tmp)[1]);

	alignas(16) __m128i offset128, tmp128, data128;
	offset128 = ((__m128i *)&offset)[3];

	for (size_t i = 0; i < b512rem; i++)
	{
		tmp128 = ((__m128i *)(&Lstbl_o[_tzcnt_u64(i + 1)]))[0];
		offset128 = _mm_xor_si128(tmp128, offset128);
		data128 = _mm_loadu_si128((__m128i *)A + (blen - b512rem + i));
		tmp128 = _mm_xor_si128(offset128, data128);
		tmp128 = aesenc128(tmp128, aesctx.keys128);
		sum = _mm_xor_si128(tmp128, sum);
	}

	if (brem > 0)
	{
		offset128 = _mm_xor_si128(offset128, ((__m128i *)&ctx.L_star)[0]);
		data128 = ozp128(brem, A + len - brem);
		tmp128 = _mm_xor_si128(data128, offset128);
		tmp128 = aesenc128(tmp128, aesctx.keys128);
		sum = _mm_xor_si128(tmp128, sum);
	}

	return sum;
}

static inline __m128i ocbhash512x4(aes_context aesctx, ocb_context ctx, const uint8_t *A, size_t len)
{
	size_t blen = len / ctx.blocklength;
	size_t b512len = blen / 4;
	size_t brem = len % ctx.blocklength;
	size_t b512rem = blen % 4;
	size_t bx4rem = b512len % 4;
	size_t bx4len = b512len / 4;

	alignas(64) __m128i sum = _mm_setzero_si128();

	alignas(64) __m512i sum512 = _mm512_setzero_si512();
	alignas(64) __m512i offset = _mm512_setzero_si512();
	alignas(64) __m512i data, tmp;

	alignas(64) __m512i data1, _data1;
	alignas(64) __m512i data2, _data2;
	alignas(64) __m512i data3, _data3;
	alignas(64) __m512i data4, _data4;

	alignas(512) __m512i L1, L3, Li, Li1, Li2;
	L1 = Lstbl[1];
	L3 = Lstbl[3];

	tmp = _mm512_xor_si512(Lstbl[4], L3);
	alignas(64) __m512i offset1 = _mm512_xor_si512(tmp, process_offset(offset, 0));
	alignas(64) __m512i offset2 = _mm512_xor_si512(tmp, process_offset(offset, 1));
	alignas(64) __m512i offset3 = _mm512_xor_si512(tmp, process_offset(offset, 2));
	tmp = _mm512_xor_si512(tmp, Lstbl2[4]);
	tmp = _mm512_xor_si512(tmp, Lstbl2[5]);
	alignas(64) __m512i offset4 = _mm512_xor_si512(tmp, process_offset(offset, 3));

	for (size_t i = 0; i < bx4len; i++)
	{
		tmp = _mm512_xor_si512(Lstbl[__tzcnt_u64(16 * i + 16)], L3);
		offset1 = _mm512_xor_si512(offset1, tmp);
		offset2 = _mm512_xor_si512(offset2, tmp);
		offset3 = _mm512_xor_si512(offset3, tmp);
		tmp = _mm512_xor_si512(tmp, Lstbl2[__tzcnt_u64(16 * +16)]);
		tmp = _mm512_xor_si512(tmp, Lstbl2[__tzcnt_u64(16 * +32)]);
		offset4 = _mm512_xor_si512(offset4, tmp);

		_data1 = _mm512_loadu_si512((__m512i *)A + 4 * i + 0);
		_data2 = _mm512_loadu_si512((__m512i *)A + 4 * i + 1);
		_data3 = _mm512_loadu_si512((__m512i *)A + 4 * i + 2);
		_data4 = _mm512_loadu_si512((__m512i *)A + 4 * i + 3);

		data1 = _mm512_xor_si512(_data1, offset1);
		data2 = _mm512_xor_si512(_data2, offset2);
		data3 = _mm512_xor_si512(_data3, offset3);
		data4 = _mm512_xor_si512(_data4, offset4);

		data1 = aesenc512(data1, aesctx.keys);
		data2 = aesenc512(data2, aesctx.keys);
		data3 = aesenc512(data3, aesctx.keys);
		data4 = aesenc512(data4, aesctx.keys);

		sum512 = _mm512_xor_si512(sum512, data1);
		sum512 = _mm512_xor_si512(sum512, data2);
		sum512 = _mm512_xor_si512(sum512, data3);
		sum512 = _mm512_xor_si512(sum512, data4);
	}

	for (size_t i = 0; i < bx4rem; i++)
	{
		Li1 = Lstbl[_tzcnt_u64(4 * i)];
		Li2 = _mm512_xor_si512(Lstbl2[_tzcnt_u64(4 * i)], Lstbl2[_tzcnt_u64(4 * i + 4)]);
		offset = _mm512_xor_si512(offset, L1);
		offset = _mm512_xor_si512(offset, Li1);
		offset = _mm512_xor_si512(offset, Li2);

		data = _mm512_loadu_si512((__m512i *)A + 4 * bx4len + i);
		tmp = _mm512_xor_si512(offset, data);
		tmp = aesenc512(tmp, aesctx.keys);
		sum512 = _mm512_xor_si512(tmp, sum512);
	}

	tmp = _mm512_setzero_si512();
	((__m128i *)&tmp)[0] = ((__m128i *)&sum512)[2];
	((__m128i *)&tmp)[1] = ((__m128i *)&sum512)[3];
	tmp = _mm512_xor_si512(sum512, tmp);
	sum = _mm_xor_si128(((__m128i *)&tmp)[0], ((__m128i *)&tmp)[1]);

	alignas(16) __m128i offset128, tmp128, data128;
	offset128 = ((__m128i *)&offset)[3];

	for (size_t i = 0; i < b512rem; i++)
	{
		tmp128 = ((__m128i *)(&Lstbl[_tzcnt_u64(i + 1)]))[0];
		offset128 = _mm_xor_si128(tmp128, offset128);
		data128 = _mm_loadu_si128((__m128i *)A + (blen - b512rem + i));
		tmp128 = _mm_xor_si128(offset128, data128);
		tmp128 = aesenc128(tmp128, aesctx.keys128);
		sum = _mm_xor_si128(tmp128, sum);
	}

	if (brem > 0)
	{
		offset128 = _mm_xor_si128(offset128, ((__m128i *)&ctx.L_star)[0]);
		data128 = ozp128(brem, A + len - brem);
		tmp128 = _mm_xor_si128(data128, offset128);
		tmp128 = aesenc128(tmp128, aesctx.keys128);
		sum = _mm_xor_si128(tmp128, sum);
	}

	return sum;
}

static inline __m128i gen_offset0(aes_context aesctx, ocb_context ctx, size_t tlen, size_t nlen, const uint8_t *N)
{
	uint8_t nonce[16];
	memset(nonce, 0, 16);
	nonce[0] = nonce[0] ^ ((uint8_t)((tlen * 8) % 128)) << 1;
	nonce[15 - nlen] ^= 0x01;
	memcpy(nonce + 16 - nlen, N, nlen);
	uint8_t bottom = nonce[15] & 0x3f;

	nonce[15] &= 0xc0;
	alignas(16) __m128i Ktop = _mm_loadu_si128((__m128i *)nonce);

	Ktop = aesenc128(Ktop, aesctx.keys128);

	uint8_t stretch[24];
	_mm_storeu_si128((__m128i *)stretch, ((__m128i *)&Ktop)[0]);
	for (size_t i = 0; i < 8; i++)
	{
		stretch[16 + i] = stretch[0 + i] ^ stretch[1 + i];
	}
	size_t bbtm = bottom / 8;
	size_t rembtm = bottom % 8;
	for (size_t i = bbtm; i < bbtm + ctx.blocklength; i++)
	{
		stretch[i] <<= rembtm;
		for (size_t j = 0; j < rembtm; j++)
		{
			stretch[i] ^= ((stretch[i + 1] << j) & 0x80) >> (8 - rembtm + j);
		}
	}
	return _mm_loadu_si128((__m128i *)(stretch + bbtm));
}

static inline void ocbenc512(aes_context aesctx, ocb_context ctx, size_t tlen, const uint8_t *N, size_t nlen, const uint8_t *A, size_t alen, const uint8_t *P, size_t plen, uint8_t *C)
{
	alignas(64) __m512i offset0 = _mm512_broadcast_i64x2(gen_offset0(aesctx, ctx, tlen, nlen, N));

	size_t blen = plen / ctx.blocklength;
	size_t b512len = blen / 4;
	size_t brem = plen % ctx.blocklength;
	size_t b512rem = blen % 4;

	alignas(64) __m128i sum = _mm_setzero_si128();

	alignas(64) __m512i sum512 = _mm512_setzero_si512();
	alignas(64) __m512i offset = offset0;
	alignas(64) __m512i data, tmp;

	alignas(64) __m512i L1, Li1, Li2;

	for (size_t i = 0; i < b512len; i++)
	{
		Li1 = Lstbl[_tzcnt_u64(4 * i)];
		Li2 = _mm512_xor_si512(Lstbl2[_tzcnt_u64(4 * i)], Lstbl2[_tzcnt_u64(4 * i + 4)]);
		offset = _mm512_xor_si512(offset, L1);
		offset = _mm512_xor_si512(offset, Li1);
		offset = _mm512_xor_si512(offset, Li2);

		data = _mm512_loadu_si512((__m512i *)P + i);
		tmp = _mm512_xor_si512(offset, data);
		tmp = aesenc512(tmp, aesctx.keys);
		tmp = _mm512_xor_si512(tmp, offset);
		_mm512_storeu_si512((__m512i *)C + i, tmp);
		sum512 = _mm512_xor_si512(data, sum512);
	}

	tmp = _mm512_setzero_si512();
	((__m128i *)&tmp)[0] = ((__m128i *)&sum512)[2];
	((__m128i *)&tmp)[1] = ((__m128i *)&sum512)[3];
	tmp = _mm512_xor_si512(sum512, tmp);
	sum = _mm_xor_si128(((__m128i *)&tmp)[0], ((__m128i *)&tmp)[1]);

	alignas(16) __m128i offset128, tmp128, data128;
	offset128 = ((__m128i *)&offset)[3];

	for (size_t i = 0; i < b512rem; i++)
	{
		tmp128 = ((__m128i *)(&Lstbl[_tzcnt_u64(i + 1)]))[0];
		offset128 = _mm_xor_si128(tmp128, offset128);
		data128 = _mm_loadu_si128((__m128i *)P + (blen - b512rem + i));
		tmp128 = _mm_xor_si128(offset128, data128);
		tmp128 = aesenc128(tmp128, aesctx.keys128);
		tmp128 = _mm_xor_si128(tmp128, offset128);
		_mm_storeu_si128((__m128i *)C + (blen - b512rem + i), tmp128);
		sum = _mm_xor_si128(data128, sum);
	}

	if (brem > 0)
	{
		offset128 = _mm_xor_si128(offset128, ((__m128i *)&ctx.L_star)[0]);
		data128 = _mm_loadu_si128((__m128i *)(P + plen - brem));
		tmp128 = aesenc128(offset128, aesctx.keys128);
		tmp128 = _mm_xor_si128(tmp128, data128);
		memcpy(C + plen - brem, ((uint8_t *)&tmp128), brem);
		sum = _mm_xor_si128(ozp128(brem, (uint8_t *)&data128), sum);
	}

	tmp128 = ((__m128i *)&ctx.L_dollar)[0];
	tmp128 = _mm_xor_si128(tmp128, offset128);
	tmp128 = _mm_xor_si128(sum, tmp128);

	alignas(16) __m128i tag = aesenc128(tmp128, aesctx.keys128);

	tmp128 = ocbhash512(aesctx, ctx, A, alen);
	tag = _mm_xor_si128(tag, tmp128);

	uint8_t tagblk[16];
	_mm_storeu_si128((__m128i *)tagblk, tag);
	memcpy(C + plen, tagblk, tlen);
}

static inline void ocbenc512x4(aes_context aesctx, ocb_context ctx, size_t tlen, const uint8_t *N, size_t nlen, const uint8_t *A, size_t alen, const uint8_t *P, size_t plen, uint8_t *C)
{
	alignas(512) __m512i offset0 = _mm512_broadcast_i64x2(gen_offset0(aesctx, ctx, tlen, nlen, N));

	size_t blen = plen / ctx.blocklength;
	size_t b512len = blen / 4;
	size_t brem = plen % ctx.blocklength;
	size_t b512rem = blen % 4;
	size_t bx4rem = b512len % 4;
	size_t bx4len = b512len / 4;

	alignas(64) __m128i sum = _mm_setzero_si128();

	alignas(64) __m512i sum512 = _mm512_setzero_si512();
	alignas(64) __m512i offset = offset0;
	alignas(64) __m512i data, tmp;

	alignas(64) __m512i offset1;
	alignas(64) __m512i offset2;
	alignas(64) __m512i offset3;
	alignas(64) __m512i offset4;

	alignas(64) __m512i data1, _data1;
	alignas(64) __m512i data2, _data2;
	alignas(64) __m512i data3, _data3;
	alignas(64) __m512i data4, _data4;

	alignas(512) __m512i L1, L3, Li, Li1, Li2;

	for (size_t i = 0; i < bx4len; i++)
	{
		tmp = _mm512_xor_si512(Lstbl[__tzcnt_u64(16 * i + 16)], L3);
		offset1 = _mm512_xor_si512(offset1, tmp);
		offset2 = _mm512_xor_si512(offset2, tmp);
		offset3 = _mm512_xor_si512(offset3, tmp);
		tmp = _mm512_xor_si512(tmp, Lstbl2[__tzcnt_u64(16 * +16)]);
		tmp = _mm512_xor_si512(tmp, Lstbl2[__tzcnt_u64(16 * +32)]);
		offset4 = _mm512_xor_si512(offset4, tmp);

		_data1 = _mm512_loadu_si512((__m512i *)P + 4 * i + 0);
		_data2 = _mm512_loadu_si512((__m512i *)P + 4 * i + 1);
		_data3 = _mm512_loadu_si512((__m512i *)P + 4 * i + 2);
		_data4 = _mm512_loadu_si512((__m512i *)P + 4 * i + 3);

		data1 = _mm512_xor_si512(_data1, offset1);
		data2 = _mm512_xor_si512(_data2, offset2);
		data3 = _mm512_xor_si512(_data3, offset3);
		data4 = _mm512_xor_si512(_data4, offset4);

		data1 = aesenc512(data1, aesctx.keys);
		data2 = aesenc512(data2, aesctx.keys);
		data3 = aesenc512(data3, aesctx.keys);
		data4 = aesenc512(data4, aesctx.keys);

		data1 = _mm512_xor_si512(data1, offset1);
		data2 = _mm512_xor_si512(data2, offset2);
		data3 = _mm512_xor_si512(data3, offset3);
		data4 = _mm512_xor_si512(data4, offset4);

		_mm512_storeu_si512((__m512i *)C + 4 * i + 0, data1);
		_mm512_storeu_si512((__m512i *)C + 4 * i + 1, data2);
		_mm512_storeu_si512((__m512i *)C + 4 * i + 2, data3);
		_mm512_storeu_si512((__m512i *)C + 4 * i + 3, data4);
	}

	for (size_t i = 0; i < bx4rem; i++)
	{
		Li1 = Lstbl[_tzcnt_u64(4 * i)];
		Li2 = _mm512_xor_si512(Lstbl2[_tzcnt_u64(4 * i)], Lstbl2[_tzcnt_u64(4 * i + 4)]);
		offset = _mm512_xor_si512(offset, L1);
		offset = _mm512_xor_si512(offset, Li1);
		offset = _mm512_xor_si512(offset, Li2);

		data = _mm512_loadu_si512((__m512i *)P + 4 * bx4len + i);
		tmp = _mm512_xor_si512(offset, data);
		tmp = aesenc512(tmp, aesctx.keys);
		tmp = _mm512_xor_si512(tmp, offset);
		_mm512_storeu_si512((__m512i *)C + 4 * bx4len + i, tmp);
	}

	tmp = _mm512_setzero_si512();
	((__m128i *)&tmp)[0] = ((__m128i *)&sum512)[2];
	((__m128i *)&tmp)[1] = ((__m128i *)&sum512)[3];
	tmp = _mm512_xor_si512(sum512, tmp);
	sum = _mm_xor_si128(((__m128i *)&tmp)[0], ((__m128i *)&tmp)[1]);

	alignas(16) __m128i offset128, tmp128, data128;
	offset128 = ((__m128i *)&offset)[3];

	for (size_t i = 0; i < b512rem; i++)
	{
		tmp128 = ((__m128i *)(&Lstbl[_tzcnt_u64(i + 1)]))[0];
		offset128 = _mm_xor_si128(tmp128, offset128);
		data128 = _mm_loadu_si128((__m128i *)P + (blen - b512rem + i));
		tmp128 = _mm_xor_si128(offset128, data128);
		tmp128 = aesenc128(tmp128, aesctx.keys128);
		tmp128 = _mm_xor_si128(tmp128, offset128);
		_mm_storeu_si128((__m128i *)C + (blen - b512rem + i), tmp128);
		sum = _mm_xor_si128(sum, data128);
	}

	if (brem > 0)
	{
		offset128 = _mm_xor_si128(offset128, ((__m128i *)&ctx.L_star)[0]);
		data128 = ozp128(brem, P + plen - brem);
		sum = _mm_xor_si128(sum, data128);
		tmp128 = aesenc128(offset128, aesctx.keys128);
		tmp128 = _mm_xor_si128(tmp128, data128);
		memcpy(C + plen - brem, ((uint8_t *)&tmp128), brem);
	}

	tmp128 = ((__m128i *)&ctx.L_dollar)[0];
	tmp128 = _mm_xor_si128(tmp128, offset128);
	tmp128 = _mm_xor_si128(sum, tmp128);

	alignas(16) __m128i tag = aesenc128(tmp128, aesctx.keys128);

	tmp128 = ocbhash512(aesctx, ctx, A, alen);
	tag = _mm_xor_si128(tag, tmp128);

	uint8_t tagblk[16];
	_mm_storeu_si128((__m128i *)tagblk, tag);
	memcpy(C + plen, tagblk, tlen);
}

static inline void ocbenc512x4_modified(aes_context aesctx, ocb_context ctx, size_t tlen, const uint8_t *N, size_t nlen, const uint8_t *A, size_t alen, const uint8_t *P, size_t plen, uint8_t *C)
{
	alignas(512) __m512i checksum = _mm512_setzero_si512();
	alignas(512) __m512i tmp;
	alignas(512) __m512i data;
	alignas(512) __m512i data1, data2, data3, data4;
	alignas(512) __m512i _data1, _data2, _data3, _data4;

	alignas(512) __m512i omega = ctx.omegafirst;
	alignas(512) __m512i omega1 = omega;
	alignas(512) __m512i omega2 = omegax16(ctx, omega1);
	alignas(512) __m512i omega3 = omegax16(ctx, omega2);
	alignas(512) __m512i omega4 = omegax16(ctx, omega3);

	size_t blen = plen / ctx.blocklength;
	size_t b512len = blen / 4;
	size_t brem = plen % ctx.blocklength;
	size_t b512rem = blen % 4;

	size_t bx4len = b512len / 4;
	size_t bx4rem = b512len % 4;

	for (size_t i = 0; i < bx4len; i++)
	{
		_data1 = _mm512_loadu_si512((__m512i *)P + 4 * i + 0);
		_data2 = _mm512_loadu_si512((__m512i *)P + 4 * i + 1);
		_data3 = _mm512_loadu_si512((__m512i *)P + 4 * i + 2);
		_data4 = _mm512_loadu_si512((__m512i *)P + 4 * i + 3);

		/*
		data1 = _mm512_xor_si512(_data1, omega1);
		data2 = _mm512_xor_si512(_data2, omega2);
		data3 = _mm512_xor_si512(_data3, omega3);
		data4 = _mm512_xor_si512(_data4, omega4);
		*/
		data1 = _mm512_xor_si512(_data1, Lstbl[4 * i]);
		data2 = _mm512_xor_si512(_data2, Lstbl[4 * i + 1]);
		data3 = _mm512_xor_si512(_data3, Lstbl[4 * i + 2]);
		data4 = _mm512_xor_si512(_data4, Lstbl[4 * i + 3]);

		data1 = aesenc512(data1, aesctx.keys);
		data2 = aesenc512(data2, aesctx.keys);
		data3 = aesenc512(data3, aesctx.keys);
		data4 = aesenc512(data4, aesctx.keys);

		/*
		data1 = _mm512_xor_si512(data1, omega1);
		data2 = _mm512_xor_si512(data2, omega2);
		data3 = _mm512_xor_si512(data3, omega3);
		data4 = _mm512_xor_si512(data4, omega4);
		*/

		data1 = _mm512_xor_si512(data1, Lstbl[4 * i]);
		data2 = _mm512_xor_si512(data2, Lstbl[4 * i + 1]);
		data3 = _mm512_xor_si512(data3, Lstbl[4 * i + 2]);
		data4 = _mm512_xor_si512(data4, Lstbl[4 * i + 3]);

		_mm512_storeu_si512((__m512i *)C + 4 * i + 0, data1);
		_mm512_storeu_si512((__m512i *)C + 4 * i + 1, data2);
		_mm512_storeu_si512((__m512i *)C + 4 * i + 2, data3);
		_mm512_storeu_si512((__m512i *)C + 4 * i + 3, data4);

		checksum = _mm512_xor_si512(checksum, _data1);
		checksum = _mm512_xor_si512(checksum, _data2);
		checksum = _mm512_xor_si512(checksum, _data3);
		checksum = _mm512_xor_si512(checksum, _data4);

		/*
		omega1 = omegax65535(ctx, omega1);
		omega2 = omegax65535(ctx, omega2);
		omega3 = omegax65535(ctx, omega3);
		omega4 = omegax65535(ctx, omega4);
		*/
	}

	omega = omega1;

	for (size_t i = 0; i < bx4rem; i++)
	{
		data = _mm512_loadu_si512((__m512i *)P + bx4len * 4 + i);
		tmp = _mm512_xor_si512(data, omega);
		tmp = aesenc512(tmp, aesctx.keys);
		tmp = _mm512_xor_si512(tmp, omega);
		_mm512_storeu_si512((__m512i *)C + bx4len * 4 + i, tmp);

		checksum = _mm512_xor_si512(checksum, data1);
		omega = omegax16(ctx, omega);
	}

	data = _mm512_setzero_si512();

	((__m128i *)&tmp)[0] = ((__m128i *)&checksum)[2];
	((__m128i *)&tmp)[1] = ((__m128i *)&checksum)[3];
	tmp = _mm512_xor_si512(checksum, tmp);
	checksum = _mm512_setzero_si512();
	((__m128i *)&checksum)[3] = _mm_xor_si128(((__m128i *)&tmp)[0], ((__m128i *)&tmp)[1]);

	for (size_t i = 0; i < b512rem; i++)
	{
		// we use only the last 128bit-block here

		((__m128i *)&data)[0] = _mm_loadu_si128((__m128i *)P + (blen - b512rem + i));
		checksum = _mm512_xor_si512(checksum, data);
		tmp = _mm512_xor_si512(omega, data);
		tmp = aesenc512(tmp, aesctx.keys);
		tmp = _mm512_xor_si512(tmp, omega);
		_mm_storeu_si128((__m128i *)C + (blen - b512rem + i), ((__m128i *)&tmp)[0]);
		omega = omegax2(ctx, omega);
	}
}