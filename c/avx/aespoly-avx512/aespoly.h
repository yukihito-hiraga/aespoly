#pragma once
#include "core.h"

//#define ONE_INTERLEAVE

inline __m128i polyreduce128(__m128i poly, __m128i x)
{
	alignas(16) __m128i x0 = _mm_clmulepi64_si128(x, poly, 0x10);
	alignas(16) __m128i y0 = _mm_shuffle_epi32(x, 78);
	alignas(16) __m128i y1 = _mm_xor_si128(y0, x0);
	alignas(16) __m128i x1 = _mm_clmulepi64_si128(y1, poly, 0x10);
	alignas(16) __m128i y2 = _mm_shuffle_epi32(y1, 78);
	return _mm_xor_si128(y2, x1);
}

inline __m128i polydot128(__m128i poly, __m128i a, __m128i b)
{
	alignas(16) __m128i pp00 = _mm_clmulepi64_si128(a, b, 0x00);
	alignas(16) __m128i pp11 = _mm_clmulepi64_si128(a, b, 0x11);
	alignas(16) __m128i pp10 = _mm_clmulepi64_si128(a, b, 0x01);
	alignas(16) __m128i pp01 = _mm_clmulepi64_si128(a, b, 0x10);

	alignas(16) __m128i ppmid = _mm_xor_si128(pp10, pp01);
	alignas(16) __m128i ppmid_ls = _mm_bslli_si128(ppmid, 8);
	alignas(16) __m128i ppmid_rs = _mm_bsrli_si128(ppmid, 8);
	alignas(16) __m128i ppupper = _mm_xor_si128(ppmid_rs, pp11);
	alignas(16) __m128i pplower = _mm_xor_si128(ppmid_ls, pp00);
	alignas(16) __m128i ppl_reduced = polyreduce128(poly, pplower);
	return _mm_xor_si128(ppupper, ppl_reduced);
}

inline __m512i polyreduce512(__m512i poly, __m512i x)
{
	alignas(64) __m512i x0 = _mm512_clmulepi64_epi128(x, poly, 0x10);
	alignas(64) __m512i y0 = _mm512_shuffle_epi32(x, 78);
	alignas(64) __m512i y1 = _mm512_xor_si512(y0, x0);
	alignas(64) __m512i x1 = _mm512_clmulepi64_epi128(y1, poly, 0x10);
	alignas(64) __m512i y2 = _mm512_shuffle_epi32(y1, 78);
	return _mm512_xor_si512(y2, x1);
}

inline __m512i polydot512(__m512i poly, __m512i a, __m512i b)
{
	alignas(64) __m512i pp00 = _mm512_clmulepi64_epi128(a, b, 0x00);
	alignas(64) __m512i pp11 = _mm512_clmulepi64_epi128(a, b, 0x11);
	alignas(64) __m512i pp10 = _mm512_clmulepi64_epi128(a, b, 0x01);
	alignas(64) __m512i pp01 = _mm512_clmulepi64_epi128(a, b, 0x10);

	alignas(64) __m512i ppmid = _mm512_xor_si512(pp10, pp01);
	alignas(64) __m512i ppmid_ls = _mm512_bslli_epi128(ppmid, 8);
	alignas(64) __m512i ppmid_rs = _mm512_bsrli_epi128(ppmid, 8);
	alignas(64) __m512i ppupper = _mm512_xor_si512(ppmid_rs, pp11);
	alignas(64) __m512i pplower = _mm512_xor_si512(ppmid_ls, pp00);
	alignas(64) __m512i ppl_reduced = polyreduce512(poly, pplower);
	return _mm512_xor_si512(ppupper, ppl_reduced);
}

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

static inline void init(aespoly_context *ctx, uint8_t *key)
{
	aesinit512(&(ctx->aes_ctx), key);
	aesinit128(&(ctx->aes_ctx), key);

	ctx->poly_double128 = _mm_setr_epi32(0x87 << 8, 0, 0, 0);
	ctx->poly128 = _mm_setr_epi32(0x1, 0, 0, 0xc2000000);
	ctx->poly_double = _mm512_broadcast_i64x2(ctx->poly_double128);
	ctx->poly = _mm512_broadcast_i64x2(ctx->poly_double128);
	ctx->L[0] = aesenc512(_mm512_setzero_si512(), ctx->aes_ctx.keys);
	ctx->omega[0] = aesenc512(_mm512_broadcast_i64x2(_mm_setr_epi32(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff)), ctx->aes_ctx.keys);

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
		((__m128i *)ctx->omega)[3 + i] = double128(ctx->poly_double128, ((__m128i *)ctx->omega)[2 + i]);
	}

	for (size_t i = 2; i < 250; i++)
	{
		ctx->L[i] = quadruple512(ctx->poly_quadruple1, ctx->poly_quadruple2, ctx->L[i - 1]);
		ctx->omega[i] = quadruple512(ctx->poly_quadruple1, ctx->poly_quadruple2, ctx->omega[i - 1]);
	}

	ctx->offsetL[0] = _mm512_setzero_si512();
	ctx->offsetomega[0] = _mm512_setzero_si512();

	for (size_t i = 1; i <= 2900; i++)
	{
		((__m128i *)ctx->offsetL)[3 + i] = _mm_xor_si128(((__m128i *)ctx->offsetL)[2 + i], ((__m128i *)ctx->L)[3 + _tzcnt_u64(i)]);
		((__m128i *)ctx->offsetomega)[3 + i] = _mm_xor_si128(((__m128i *)ctx->offsetomega)[2 + i], ((__m128i *)ctx->omega)[3 + _tzcnt_u64(i)]);
	}

	((__m128i *)ctx->htbl)[15] = aesenc128(_mm_setr_epi32(0, 0, 0, 0), ctx->aes_ctx.keys128);

	for (size_t i = 1; i <= 16; i++)
	{
		((__m128i *)ctx->htbl)[15 - i] = aesenc128(((__m128i *)ctx->htbl)[16 - i], ctx->aes_ctx.keys128);
	}
}

static inline __m128i phash512(aespoly_context* ctx, const uint8_t *M, size_t Mlen)
{
	size_t blen = Mlen / 64;
	size_t brem = Mlen % 64;

	alignas(64) __m512i data[4];
	alignas(64) __m512i tmps[4];
	alignas(64) __m512i sum = _mm512_setzero_si512();

	for (size_t i = 0; i < blen; i++)
	{
		data[0] = _mm512_loadu_si512((__m512i *)M + i);
		tmps[0] = _mm512_xor_si512(data[0], ctx->L[i + 1]);
		tmps[0] = aesenc512(tmps[0], ctx->aes_ctx.keys);
		sum = _mm512_xor_si512(sum, tmps[0]);
	}

	alignas(16) __m128i sum128 = _mm_xor_si128(_mm_xor_si128(((__m128i *)&sum)[0], ((__m128i *)&sum)[1]), _mm_xor_si128(((__m128i *)&sum)[2], ((__m128i *)&sum)[3]));

	if (brem > 0)
	{
		size_t rlen = brem / 16;
		size_t rrem = brem % 16;

		alignas(16) __m128i data128[2];
		alignas(16) __m128i tmps128[2];

		for (size_t j = 0; j < rlen; j++)
		{
			data128[0] = _mm_loadu_si128((__m128i *)M + j);
			tmps128[0] = _mm_xor_si128(data128[0], ((__m128i *)ctx->L)[4 * blen + j + 4]);
			tmps128[0] = aesenc128(tmps128[0], ctx->aes_ctx.keys128);
			sum128 = _mm_xor_si128(sum128, tmps128[0]);
		}
	}

	return sum128;
}

static inline void tweaker(aespoly_context* ctx, const uint8_t *T, size_t Tlen, __m128i *state, __m128i *hash)
{
	size_t t = (Tlen + 15) / 16;
	size_t rem = Tlen + 16 - t * 16;
	size_t dlen = MAX(t - t % 2 - 2, 0) / 2;
	size_t d512rem = dlen % 4;
	size_t d512len = dlen / 4;
	size_t d512x4rem = d512len % 4;
	size_t d512x4len = d512len / 4;

	alignas(64) __m512i data[8];
	alignas(64) __m512i tmps[12];

	__m512i *dataL = data;
	__m512i *dataR = data + 4;

	__m512i *tmpsL = tmps;
	__m512i *tmpsR = tmps + 4;
	__m512i *mask = tmps + 8;

	alignas(64) __m512i X = _mm512_setzero_si512();
	alignas(64) __m512i Y = _mm512_setzero_si512();
	alignas(64) __m512i Z = _mm512_setzero_si512();

	alignas(64) __m512i sum = _mm512_setzero_si512();

	for (size_t i = 0; i < d512x4len; i++)
	{
		gather_load_x16((__m128i *)T + i * 32, (__m128i *)dataR);
		gather_load_x16((__m128i *)T + i * 32 + 1, (__m128i *)dataL);

		Y = polyreduce512(ctx->poly, Y);
		Z = _mm512_xor_si512(X, Y);
		Z = _mm512_xor_si512(Z, data[0]);

		schoolbook_initialadd512(tmpsL, dataL[3], ctx->htbl[3]);
		schoolbook_add512(tmpsL, dataL[2], ctx->htbl[2]);
		schoolbook_add512(tmpsL, dataL[1], ctx->htbl[1]);
		schoolbook_add512(tmpsL, Z, ctx->htbl[0]);

		tmpsR[0] = aesenc512(_mm512_xor_si512(dataR[0], ctx->offsetL[4 * i + 0 + 1]), ctx->aes_ctx.keys);
		tmpsR[1] = aesenc512(_mm512_xor_si512(dataR[1], ctx->offsetL[4 * i + 1 + 1]), ctx->aes_ctx.keys);
		tmpsR[2] = aesenc512(_mm512_xor_si512(dataR[2], ctx->offsetL[4 * i + 2 + 1]), ctx->aes_ctx.keys);
		tmpsR[3] = aesenc512(_mm512_xor_si512(dataR[3], ctx->offsetL[4 * i + 3 + 1]), ctx->aes_ctx.keys);

		sum = _mm512_xor_si512(_mm512_xor_si512(tmpsR[0], tmpsR[1]), _mm512_xor_si512(tmpsR[2], tmpsR[3]));

		tmpsL[3] = _mm512_bsrli_epi128(tmpsL[2], 8);
		tmpsL[2] = _mm512_bslli_epi128(tmpsL[2], 8);

		X = _mm512_xor_si512(tmpsL[3], tmpsL[1]);
		Y = _mm512_xor_si512(tmpsL[0], tmpsL[2]);
	}

	Y = polyreduce512(ctx->poly, Y);
	Z = _mm512_xor_si512(X, Y);
	X = Z;

	for (size_t i = d512x4len * 4; i < d512len; i++)
	{
		gather_load_x4((__m128i *)T + i * 8, (__m128i *)dataR);
		gather_load_x4((__m128i *)T + i * 8 + 1, (__m128i *)dataL);

		tmpsR[0] = aesenc512(_mm512_xor_si512(dataR[0], ctx->offsetL[i + 1]), ctx->aes_ctx.keys);
		sum = _mm512_xor_si512(tmpsR[0], sum);
		tmpsL[0] = _mm512_xor_si512(dataL[0], X);
		X = polydot512(ctx->poly, X, ctx->htbl[15]);
	}

	alignas(16) __m128i data128[8];
	alignas(16) __m128i tmps128[12];

	__m128i *dataL128 = data128;
	__m128i *dataR128 = data128 + 4;

	__m128i *tmpsL128 = tmps128;
	__m128i *tmpsR128 = tmps128 + 4;
	__m128i *mask128 = tmps128 + 8;

	alignas(16) __m128i X128 = _mm_xor_si128(_mm_xor_si128(((__m128i *)&X)[0], ((__m128i *)&X)[1]), _mm_xor_si128(((__m128i *)&X)[2], ((__m128i *)&X)[3]));
	alignas(16) __m128i sum128 = _mm_xor_si128(_mm_xor_si128(((__m128i *)&sum)[0], ((__m128i *)&sum)[1]), _mm_xor_si128(((__m128i *)&sum)[2], ((__m128i *)&sum)[3]));

	for (size_t i = d512len * 4; i < dlen; i++)
	{
		gather_load_x1((__m128i *)T + i * 2, dataR128);
		gather_load_x1((__m128i *)T + i * 2 + 1, dataL128);

		tmpsR128[0] = aesenc128(_mm_xor_si128(dataR128[0], ((__m128i *)ctx->offsetL)[i + 4]), ctx->aes_ctx.keys128);
		tmpsL128[0] = _mm_xor_si128(dataL128[0], X128);
		X128 = polydot128(ctx->poly128, tmpsL128[0], ((__m128i *)ctx->htbl)[15]);
	}

	if (Tlen == 16)
	{
		state[0] = X128;
		hash[0] = _mm_loadu_si128((__m128i *)T);
	}
	else
	{
		if (t % 2 == 0)
		{
			data128[0] = _mm_loadu_si128((__m128i *)T + dlen * 2);

			uint8_t padded[16];
			memset(padded, 0, 16);
			memcpy(padded, T + Tlen - rem, rem);
			// padded[rem] = 0x80;
			alignas(16) __m128i paddedblk = _mm_loadu_si128((__m128i *)padded);

			tmps128[0] = _mm_xor_si128(data128[0], ((__m128i *)ctx->offsetL)[dlen + 4]);
			tmps128[0] = aesenc128(tmps128[0], ctx->aes_ctx.keys128);
			hash[0] = _mm_xor_si128(tmps128[0], sum128);

			X128 = _mm_xor_si128(X128, paddedblk);
			state[0] = polydot128(ctx->poly128, X128, ((__m128i *)ctx->htbl)[15]);
		}
		else
		{
			data128[0] = _mm_loadu_si128((__m128i *)T + dlen * 2);
			data128[2] = _mm_loadu_si128((__m128i *)(T + Tlen - 16));

			uint8_t padded[16];
			memset(padded, 0, 16);
			memcpy(padded, T + Tlen - 16 - rem, rem);
			// padded[rem] = 0x80;
			alignas(16) __m128i paddedblk = _mm_loadu_si128((__m128i *)padded);

			tmps128[0] = _mm_xor_si128(data128[0], ((__m128i *)ctx->offsetL)[dlen + 4]);
			tmps128[0] = aesenc128(tmps128[0], ctx->aes_ctx.keys128);
			sum128 = _mm_xor_si128(tmps128[0], sum128);

			tmps128[0] = _mm_xor_si128(data128[2], ((__m128i *)ctx->offsetL)[dlen + 5]);
			tmps128[0] = aesenc128(tmps128[0], ctx->aes_ctx.keys128);
			sum128 = _mm_xor_si128(tmps128[0], sum128);

			X128 = _mm_xor_si128(X128, paddedblk);
			state[0] = polydot128(ctx->poly128, X128, ((__m128i *)ctx->htbl)[15]);
			hash[0] = _mm_xor_si128(sum128, data128[2]);
		}
	}
}

static inline void upper(aespoly_context* ctx, __m128i state, const uint8_t *M, size_t Mlen, uint8_t *C, __m128i *hash, __m128i *sum128)
{
	size_t m = (Mlen + 15) / 16;
	size_t rem = Mlen + 16 - m * 16;
	size_t dlen = MAX(m - m % 2 - 2, 0) / 2;
	size_t d512rem = dlen % 4;
	size_t d512len = dlen / 4;
	size_t d512x4rem = d512len % 4;
	size_t d512x4len = d512len / 4;

	alignas(64) __m512i data[8];
	alignas(64) __m512i tmps[12];

	__m512i *dataL = data;
	__m512i *dataR = data + 4;

	__m512i *tmpsL = tmps;
	__m512i *tmpsR = tmps + 4;
	__m512i *mask = tmps + 8;

	alignas(64) __m512i X = _mm512_setzero_si512();
	((__m128i *)&X)[0] = state;
	alignas(64) __m512i Y = _mm512_setzero_si512();
	alignas(64) __m512i Z = _mm512_setzero_si512();

	alignas(64) __m512i sum = _mm512_setzero_si512();

	for (size_t i = 0; i < d512x4len; i++)
	{

		#ifdef ONE_INTERLEAVE
		gather_load_x16((__m128i *)M + i * 32 + 1, (__m128i *)dataR);
		gather_load_x16((__m128i *)M + i * 32, (__m128i *)dataL);
		#else
		dataL[0] = _mm512_loadu_si512((__m512i *)M + i * 8 + 0);
		dataL[1] = _mm512_loadu_si512((__m512i *)M + i * 8 + 1);
		dataL[2] = _mm512_loadu_si512((__m512i *)M + i * 8 + 2);
		dataL[3] = _mm512_loadu_si512((__m512i *)M + i * 8 + 3);
		dataR[0] = _mm512_loadu_si512((__m512i *)M + i * 8 + 4 + 0);
		dataR[1] = _mm512_loadu_si512((__m512i *)M + i * 8 + 4 + 1);
		dataR[2] = _mm512_loadu_si512((__m512i *)M + i * 8 + 4 + 2);
		dataR[3] = _mm512_loadu_si512((__m512i *)M + i * 8 + 4 + 3);

		//gather_x16((__m128i *)tmps, (__m128i *)dataL);
		//gather_x16((__m128i *)tmps + 1, (__m128i *)dataR);
		#endif

		Y = polyreduce512(ctx->poly, Y);
		Z = _mm512_xor_si512(X, Y);
		Z = _mm512_xor_si512(Z, dataL[0]);

		schoolbook_initialadd512(tmpsL, dataL[3], ctx->htbl[3]);
		schoolbook_add512(tmpsL, dataL[2], ctx->htbl[2]);
		schoolbook_add512(tmpsL, dataL[1], ctx->htbl[1]);
		schoolbook_add512(tmpsL, Z, ctx->htbl[0]);

		tmpsR[0] = aesenc512(_mm512_xor_si512(dataR[0], ctx->offsetL[4 * i + 0 + 1]), ctx->aes_ctx.keys);
		tmpsR[1] = aesenc512(_mm512_xor_si512(dataR[1], ctx->offsetL[4 * i + 1 + 1]), ctx->aes_ctx.keys);
		tmpsR[2] = aesenc512(_mm512_xor_si512(dataR[2], ctx->offsetL[4 * i + 2 + 1]), ctx->aes_ctx.keys);
		tmpsR[3] = aesenc512(_mm512_xor_si512(dataR[3], ctx->offsetL[4 * i + 3 + 1]), ctx->aes_ctx.keys);

		sum = _mm512_xor_si512(_mm512_xor_si512(tmpsR[0], tmpsR[1]), _mm512_xor_si512(tmpsR[2], tmpsR[3]));

		tmpsL[3] = _mm512_bsrli_epi128(tmpsL[2], 8);
		tmpsL[2] = _mm512_bslli_epi128(tmpsL[2], 8);

		X = _mm512_xor_si512(tmpsL[3], tmpsL[1]);
		Y = _mm512_xor_si512(tmpsL[0], tmpsL[2]);

		#ifdef ONE_INTERLEAVE
		scatter_store_x16((__m128i *)C + i * 32 + 1, (__m128i *)tmpsR);
		#else
		_mm512_storeu_si512((__m512i *)C + i * 8 + 4 + 0, tmpsR[0]);
		_mm512_storeu_si512((__m512i *)C + i * 8 + 4 + 1, tmpsR[1]);
		_mm512_storeu_si512((__m512i *)C + i * 8 + 4 + 2, tmpsR[2]);
		_mm512_storeu_si512((__m512i *)C + i * 8 + 4 + 3, tmpsR[3]);
		#endif
	}

	Y = polyreduce512(ctx->poly, Y);
	Z = _mm512_xor_si512(X, Y);
	X = Z;

	for (size_t i = d512x4len * 4; i < d512len; i++)
	{
		gather_load_x4((__m128i *)M + i * 8 + 1, (__m128i *)dataR);
		gather_load_x4((__m128i *)M + i * 8, (__m128i *)dataL);

		tmpsR[0] = aesenc512(_mm512_xor_si512(dataR[0], ctx->offsetL[i + 1]), ctx->aes_ctx.keys);
		sum = _mm512_xor_si512(tmpsR[0], sum);
		tmpsL[0] = _mm512_xor_si512(dataL[0], X);
		X = polydot512(ctx->poly, X, ctx->htbl[15]);

		scatter_store_x4((__m128i *)C + i * 8 + 1, (__m128i *)tmpsR);
	}

	alignas(16) __m128i data128[8];
	alignas(16) __m128i tmps128[12];

	__m128i *dataL128 = data128;
	__m128i *dataR128 = data128 + 4;

	__m128i *tmpsL128 = tmps128;
	__m128i *tmpsR128 = tmps128 + 4;
	__m128i *mask128 = tmps128 + 8;

	alignas(16) __m128i X128 = _mm_xor_si128(_mm_xor_si128(((__m128i *)&X)[0], ((__m128i *)&X)[1]), _mm_xor_si128(((__m128i *)&X)[2], ((__m128i *)&X)[3]));
	sum128[0] = _mm_xor_si128(_mm_xor_si128(((__m128i *)&sum)[0], ((__m128i *)&sum)[1]), _mm_xor_si128(((__m128i *)&sum)[2], ((__m128i *)&sum)[3]));

	for (size_t i = d512len * 4; i < dlen; i++)
	{
		gather_load_x1((__m128i *)M + i * 2 + 1, dataR128);
		gather_load_x1((__m128i *)M + i * 2, dataL128);

		tmpsR128[0] = aesenc128(_mm_xor_si128(dataR128[0], ((__m128i *)ctx->offsetL)[i + 4]), ctx->aes_ctx.keys128);
		sum128[0] = _mm_xor_si128(sum128[0], tmpsR128[0]);
		tmpsL128[0] = _mm_xor_si128(dataL128[0], X128);
		X128 = polydot128(ctx->poly128, tmpsL128[0], ((__m128i *)ctx->htbl)[15]);

		scatter_store_x1((__m128i *)C + i * 2, tmpsR128);
	}

	if (Mlen == 16)
	{
		sum128[0] = X128;
		hash[0] = _mm_loadu_si128((__m128i *)M);
	}
	else
	{
		if (m % 2 == 0)
		{
			data128[0] = _mm_loadu_si128((__m128i *)M + dlen * 2);

			uint8_t padded[16];
			memset(padded, 0, 16);
			memcpy(padded, M + Mlen - rem, rem);
			// padded[rem] = 0x80;
			alignas(16) __m128i paddedblk = _mm_loadu_si128((__m128i *)padded);

			tmps128[0] = _mm_xor_si128(data128[0], ((__m128i *)ctx->offsetomega)[dlen + 4]);
			tmps128[0] = aesenc128(tmps128[0], ctx->aes_ctx.keys128);
			sum128[0] = _mm_xor_si128(tmps128[0], sum128[0]);

			X128 = _mm_xor_si128(X128, paddedblk);
			hash[0] = polydot128(ctx->poly128, X128, ((__m128i *)ctx->htbl)[15]);
		}
		else
		{
			data128[0] = _mm_loadu_si128((__m128i *)M + dlen * 2);
			data128[2] = _mm_loadu_si128((__m128i *)(M + Mlen - 16));

			uint8_t padded[16];
			memset(padded, 0, 16);
			memcpy(padded, M + Mlen - 16 - rem, rem);
			// padded[rem] = 0x80;
			alignas(16) __m128i paddedblk = _mm_loadu_si128((__m128i *)padded);

			tmps128[0] = _mm_xor_si128(data128[0], ((__m128i *)ctx->offsetomega)[dlen + 4]);
			tmps128[0] = aesenc128(tmps128[0], ctx->aes_ctx.keys128);
			sum128[0] = _mm_xor_si128(tmps128[0], sum128[0]);

			tmps128[0] = _mm_xor_si128(data128[2], ((__m128i *)ctx->offsetomega)[dlen + 5]);
			tmps128[0] = aesenc128(tmps128[0], ctx->aes_ctx.keys128);
			sum128[0] = _mm_xor_si128(tmps128[0], sum128[0]);

			X128 = _mm_xor_si128(X128, paddedblk);
			hash[0] = polydot128(ctx->poly128, X128, ((__m128i *)ctx->htbl)[15]);
			sum128[0] = _mm_xor_si128(sum128[0], data128[2]);
		}
	}
}

static inline __m128i middlelower(aespoly_context* ctx, __m128i state, __m128i S1, __m128i S2, const uint8_t *M, size_t Mlen, uint8_t *C)
{
	size_t m = (Mlen + 15) / 16;
	size_t rem = Mlen + 16 - m * 16;
	size_t dlen = MAX(m - m % 2 - 2, 0) / 2;
	size_t d512rem = dlen % 4;
	size_t d512len = dlen / 4;
	size_t d512x4rem = d512len % 4;
	size_t d512x4len = d512len / 4;

	alignas(64) __m512i data[8];
	alignas(64) __m512i tmps[12];
	alignas(64) __m512i ctr[4];
	alignas(64) __m512i inc16 = _mm512_broadcast_i64x2(_mm_setr_epi32(16, 0, 0, 0));
	alignas(64) __m512i inc4 = _mm512_broadcast_i64x2(_mm_setr_epi32(4, 0, 0, 0));

	for (size_t i = 0; i < 16; i++)
	{
		((__m128i *)ctr)[i] = _mm_setr_epi32(i + 1, 0, 0, 0);
	}

	__m512i *dataL = data;
	__m512i *dataR = data + 4;

	__m512i *tmpsL = tmps;
	__m512i *tmpsR = tmps + 4;
	__m512i *mask = tmps + 8;

	alignas(64) __m512i X = _mm512_setzero_si512();
	((__m128i *)&X)[0] = state;
	alignas(64) __m512i Y = _mm512_setzero_si512();
	alignas(64) __m512i Z = _mm512_setzero_si512();

	alignas(64) __m512i SS1 = _mm512_broadcast_i64x2(S1);
	alignas(64) __m512i SS2 = _mm512_broadcast_i64x2(S2);

	for (size_t i = 0; i < d512x4len; i++)
	{
		#ifdef ONE_INTERLEAVE
		gather_load_x16((__m128i *)M + i * 32 + 1, (__m128i *)dataR);
		gather_load_x16((__m128i *)M + i * 32, (__m128i *)dataL);
		#else
		dataL[0] = _mm512_loadu_si512((__m512i *)M + i * 8 + 0);
		dataL[1] = _mm512_loadu_si512((__m512i *)M + i * 8 + 1);
		dataL[2] = _mm512_loadu_si512((__m512i *)M + i * 8 + 2);
		dataL[3] = _mm512_loadu_si512((__m512i *)M + i * 8 + 3);
		dataR[0] = _mm512_loadu_si512((__m512i *)M + i * 8 + 4 + 0);
		dataR[1] = _mm512_loadu_si512((__m512i *)M + i * 8 + 4 + 1);
		dataR[2] = _mm512_loadu_si512((__m512i *)M + i * 8 + 4 + 2);
		dataR[3] = _mm512_loadu_si512((__m512i *)M + i * 8 + 4 + 3);
		// gather_x16((__m128i *)tmps, (__m128i *)dataL);
		// gather_x16((__m128i *)tmps + 1, (__m128i *)dataR);
		#endif

		tmpsL[0] = _mm512_xor_si512(ctr[0], SS1);
		tmpsL[1] = _mm512_xor_si512(ctr[1], SS1);
		tmpsL[2] = _mm512_xor_si512(ctr[2], SS1);
		tmpsL[3] = _mm512_xor_si512(ctr[3], SS1);

		tmpsL[0] = _mm512_xor_si512(tmpsL[0], dataL[0]);
		tmpsL[1] = _mm512_xor_si512(tmpsL[1], dataL[1]);
		tmpsL[2] = _mm512_xor_si512(tmpsL[2], dataL[2]);
		tmpsL[3] = _mm512_xor_si512(tmpsL[3], dataL[3]);

		dataL[0] = aesenc512(tmpsL[0], ctx->aes_ctx.keys);
		dataL[1] = aesenc512(tmpsL[1], ctx->aes_ctx.keys);
		dataL[2] = aesenc512(tmpsL[2], ctx->aes_ctx.keys);
		dataL[3] = aesenc512(tmpsL[3], ctx->aes_ctx.keys);

		Y = polyreduce512(ctx->poly, Y);
		Z = _mm512_xor_si512(X, Y);
		Z = _mm512_xor_si512(Z, data[0]);

		schoolbook_initialadd512(tmpsL, dataL[3], ctx->htbl[3]);
		schoolbook_add512(tmpsL, dataL[2], ctx->htbl[2]);
		schoolbook_add512(tmpsL, dataL[1], ctx->htbl[1]);
		schoolbook_add512(tmpsL, Z, ctx->htbl[0]);

		tmpsR[0] = _mm512_xor_si512(dataR[0], SS2);
		tmpsR[1] = _mm512_xor_si512(dataR[1], SS2);
		tmpsR[2] = _mm512_xor_si512(dataR[2], SS2);
		tmpsR[3] = _mm512_xor_si512(dataR[3], SS2);

		tmpsR[0] = _mm512_xor_si512(aesenc512(tmpsR[0], ctx->aes_ctx.keys), ctx->offsetL[4 * i + 0 + 1]);
		tmpsR[1] = _mm512_xor_si512(aesenc512(tmpsR[1], ctx->aes_ctx.keys), ctx->offsetL[4 * i + 1 + 1]);
		tmpsR[2] = _mm512_xor_si512(aesenc512(tmpsR[2], ctx->aes_ctx.keys), ctx->offsetL[4 * i + 2 + 1]);
		tmpsR[3] = _mm512_xor_si512(aesenc512(tmpsR[3], ctx->aes_ctx.keys), ctx->offsetL[4 * i + 3 + 1]);

		tmpsL[3] = _mm512_bsrli_epi128(tmpsL[2], 8);
		tmpsL[2] = _mm512_bslli_epi128(tmpsL[2], 8);

		X = _mm512_xor_si512(tmpsL[3], tmpsL[1]);
		Y = _mm512_xor_si512(tmpsL[0], tmpsL[2]);

		#ifdef ONE_INTERLEAVE
		scatter_store_x16((__m128i *)C + i * 32 + 1, (__m128i *)tmpsR);
		scatter_store_x16((__m128i *)C + i * 32, (__m128i *)dataL);
		#else
		//scatter_move_x16((__m128i *)tmps, (__m128i *)dataL);
		//scatter_move_x16((__m128i *)tmps + 1, (__m128i *)dataR);
		_mm512_storeu_si512((__m512i *)C + i * 8 + 0 + 0, tmpsL[0]);
		_mm512_storeu_si512((__m512i *)C + i * 8 + 0 + 1, tmpsL[1]);
		_mm512_storeu_si512((__m512i *)C + i * 8 + 0 + 2, tmpsL[2]);
		_mm512_storeu_si512((__m512i *)C + i * 8 + 0 + 3, tmpsL[3]);
		_mm512_storeu_si512((__m512i *)C + i * 8 + 4 + 0, tmpsR[0]);
		_mm512_storeu_si512((__m512i *)C + i * 8 + 4 + 1, tmpsR[1]);
		_mm512_storeu_si512((__m512i *)C + i * 8 + 4 + 2, tmpsR[2]);
		_mm512_storeu_si512((__m512i *)C + i * 8 + 4 + 3, tmpsR[3]);
		#endif

		ctr[0] = _mm512_add_epi64(ctr[0], inc16);
		ctr[1] = _mm512_add_epi64(ctr[1], inc16);
		ctr[2] = _mm512_add_epi64(ctr[2], inc16);
		ctr[3] = _mm512_add_epi64(ctr[3], inc16);
	}

	Y = polyreduce512(ctx->poly, Y);
	Z = _mm512_xor_si512(X, Y);
	X = Z;

	for (size_t i = d512x4len * 4; i < d512len; i++)
	{
		gather_load_x4((__m128i *)M + i * 8 + 1, (__m128i *)dataR);
		gather_load_x4((__m128i *)M + i * 8, (__m128i *)dataL);

		tmpsL[0] = _mm512_xor_si512(ctr[0], SS1);
		tmpsL[0] = aesenc512(tmpsL[0], ctx->aes_ctx.keys);
		dataL[0] = _mm512_xor_si512(dataL[0], tmpsL[0]);

		tmpsR[0] = _mm512_xor_si512(dataR[0], SS2);
		tmpsR[0] = _mm512_xor_si512(aesenc512(tmpsR[0], ctx->aes_ctx.keys), ctx->offsetL[i + 1]);

		tmpsL[0] = _mm512_xor_si512(dataL[0], X);
		X = polydot512(ctx->poly, X, ctx->htbl[15]);

		scatter_store_x4((__m128i *)C + i * 8 + 1, (__m128i *)tmpsR);
		scatter_store_x4((__m128i *)C + i * 8, (__m128i *)tmpsL);

		ctr[0] = _mm512_add_epi64(ctr[0], inc4);
	}

	alignas(16) __m128i data128[8];
	alignas(16) __m128i tmps128[12];

	__m128i *dataL128 = data128;
	__m128i *dataR128 = data128 + 4;

	__m128i *tmpsL128 = tmps128;
	__m128i *tmpsR128 = tmps128 + 4;
	__m128i *mask128 = tmps128 + 8;

	alignas(16) __m128i X128 = _mm_xor_si128(_mm_xor_si128(((__m128i *)&X)[0], ((__m128i *)&X)[1]), _mm_xor_si128(((__m128i *)&X)[2], ((__m128i *)&X)[3]));
	alignas(16) __m128i inc = _mm_setr_epi32(1, 0, 0, 0);
	alignas(16) __m128i ctr128 = ((__m128i *)ctr)[0];

	for (size_t i = d512len * 4; i < dlen; i++)
	{
		gather_load_x1((__m128i *)M + i * 2 + 1, dataR128);
		gather_load_x1((__m128i *)M + i * 2, dataL128);

		tmpsR128[0] = _mm_xor_si128(dataR128[0], S2);
		tmpsR128[0] = aesenc128(tmpsR128[0], ctx->aes_ctx.keys128);
		tmpsR128[0] = _mm_xor_si128(tmpsR128[0], ((__m128i *)ctx->offsetL)[i + 4]);
		tmpsL128[0] = _mm_xor_si128(dataL128[0], X128);
		X128 = polydot128(ctx->poly128, tmpsL128[0], ((__m128i *)ctx->htbl)[15]);

		scatter_store_x1((__m128i *)C, tmpsR128);
		ctr128 = _mm_add_epi64(ctr128, inc);
	}

	if (m % 2 == 0)
	{
		dataR128[0] = _mm_loadu_si128((__m128i *)(M + Mlen - 16));

		uint8_t padded[17];
		memset(padded, 0, 16);
		memcpy(padded, M + Mlen - 16 - rem, rem);
		padded[rem] = 0x80;
		dataL128[0] = _mm_loadu_si128((__m128i *)padded);

		tmpsL128[0] = _mm_xor_si128(ctr128, S1);
		tmpsL128[0] = aesenc128(tmpsL128[0], ctx->aes_ctx.keys128);
		tmpsL128[0] = _mm_xor_si128(dataL128[0], tmpsL128[0]);

		tmpsR128[0] = _mm_xor_si128(dataR128[0], S2);

		tmpsR128[0] = aesenc128(tmpsR128[0], ctx->aes_ctx.keys128);
		tmpsR128[0] = _mm_xor_si128(tmpsR128[0], ((__m128i *)ctx->offsetomega)[dlen + 4]);

		_mm_storeu_si128((__m128i *)(C + Mlen - 16), tmpsR128[0]);

		_mm_storeu_si128((__m128i *)padded, tmpsL128[0]);
		memcpy(C + Mlen - 16 - rem, padded, rem);

		X128 = _mm_xor_si128(X128, tmpsL128[0]);
		return polydot128(ctx->poly128, X128, ((__m128i *)ctx->htbl)[15]);
	}
	else
	{
		uint8_t padded[17];
		memset(padded, 0, 16);
		memcpy(padded, M + Mlen - rem, rem);
		padded[rem] = 0x80;
		dataL128[0] = _mm_loadu_si128((__m128i *)padded);

		tmpsL128[0] = _mm_xor_si128(ctr128, S1);

		(tmpsL128)[0] = aesenc128((tmpsL128)[0], ctx->aes_ctx.keys128);
		(tmpsL128)[0] = _mm_xor_si128((dataL128)[0], (tmpsL128)[0]);

		_mm_storeu_si128((__m128i *)padded, tmpsL128[0]);
		memcpy(C + Mlen - 16 - rem, padded, rem);

		X128 = _mm_xor_si128(X128, tmpsL128[0]);
		return polydot128(ctx->poly128, X128, ((__m128i *)ctx->htbl)[15]);
	}
}

static inline void enc(aespoly_context* ctx, uint8_t *T, size_t Tlen, uint8_t *M, size_t Mlen, uint8_t *C)
{
	uint8_t *MN = M;
	uint8_t *MLR = M + 16;
	uint8_t *CN = C;
	uint8_t *CLR = C + 16;

	size_t MLRlen = Mlen - 16;
	if(Mlen < 16){
		MLRlen = 0;
	}

	alignas(16) __m128i tmps[8];

	bool mln = MLRlen % 16 > 0;

	tweaker(ctx, T, Tlen, tmps, tmps + 1);

	tmps[2] = _mm_loadu_si128((__m128i *)MN);
	tmps[2] = _mm_xor_si128(tmps[1], tmps[2]);

	upper(ctx, tmps[0], MLR, MLRlen, CLR, tmps + 3, tmps + 4);

	tmps[2] = _mm_xor_si128(tmps[2], tmps[3]);
	tmps[2] = _mm_xor_si128(tmps[2], tmps[4]);

	tmps[2] = aesenc128(tmps[2], ctx->aes_ctx.keys128);
	tmps[6] = tmps[2];

	tmps[5] = middlelower(ctx, tmps[0], tmps[2], tmps[2], CLR, MLRlen, CLR);

	tmps[2] = aesenc128(tmps[2], ctx->aes_ctx.keys128);

	tmps[2] = _mm_xor_si128(tmps[5], tmps[2]);
	tmps[2] = _mm_xor_si128(tmps[1], tmps[2]);

	if ((MLRlen / 16) % 4 > 0)
	{
		tmps[2] = _mm_xor_si128(tmps[6], tmps[2]);
	}

	_mm_storeu_si128((__m128i *)C, tmps[2]);
}