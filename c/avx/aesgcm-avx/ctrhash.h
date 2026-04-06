#pragma once
#include "../common/common.h"
#include "core.h"
#include "hash.h"

#define ctrxor(key, ctr, data, tmps, i)                  \
	{                                                    \
		(tmps)[i] = aesenc128((ctr)[i], key);            \
		(data)[i] = _mm_xor_si128((tmps)[i], (data)[i]); \
	}

static inline __m128i ctrhash128x4(aesgcm_context ctx, uint8_t *A, size_t Alen, __m128i S, uint8_t *M, size_t Mlen, uint8_t *C)
{

	alignas(16) __m128i X = ghash128x4(ctx, A, Alen, 0, 0);

	alignas(16) __m128i ctr[4];
	ctr[0] = _mm_add_epi32(_mm_setr_epi32(0, 0, 0, 0x01000000), S);
	ctr[1] = _mm_add_epi32(_mm_setr_epi32(0, 0, 0, 0x02000000), S);
	ctr[2] = _mm_add_epi32(_mm_setr_epi32(0, 0, 0, 0x03000000), S);
	ctr[3] = _mm_add_epi32(_mm_setr_epi32(0, 0, 0, 0x04000000), S);

	alignas(16) __m128i inc4 = _mm_setr_epi32(0, 0, 0, 0x04000000);
	alignas(16) __m128i inc =  _mm_setr_epi32(0, 0, 0, 0x01000000);

	size_t blen = Mlen / 16;
	size_t bx4len = blen / 4;
	size_t bx4rem = blen % 4;
	size_t Mrem = Mlen % 16;

	alignas(16) __m128i tmps[4];
	alignas(16) __m128i data[4];
	alignas(16) __m128i Y = _mm_setzero_si128(), Z = _mm_setzero_si128();

	for (size_t i = 0; i < bx4len; i++)
	{
		loadx4((__m128i *)M + i * 4, data);

		Y = polyreduce128(ctx, Y);
		Z = _mm_xor_si128(X, Y);

		tmps[3] = aesenc128(ctr[3], ctx.aesctx.keys128);
		tmps[2] = aesenc128(ctr[2], ctx.aesctx.keys128);
		tmps[1] = aesenc128(ctr[1], ctx.aesctx.keys128);
		tmps[0] = aesenc128(ctr[0], ctx.aesctx.keys128);
		data[3] = _mm_xor_si128(tmps[3], data[3]);
		data[2] = _mm_xor_si128(tmps[2], data[2]);
		data[1] = _mm_xor_si128(tmps[1], data[1]);
		data[0] = _mm_xor_si128(tmps[0], data[0]);

		Z = _mm_xor_si128(Z, byterev(data[0]));

		schoolbook_initialadd128(byterev(data[3]), ctx.htbl[0], tmps);
		schoolbook_add128(byterev(data[2]), ctx.htbl[1], tmps);
		schoolbook_add128(byterev(data[1]), ctx.htbl[2], tmps);
		schoolbook_add128(Z, ctx.htbl[3], tmps);

		storex4((__m128i *)C + i*4, data);

		ctr[0] = _mm_add_epi32(ctr[0], inc4);
		ctr[1] = _mm_add_epi32(ctr[1], inc4);
		ctr[2] = _mm_add_epi32(ctr[2], inc4);
		ctr[3] = _mm_add_epi32(ctr[3], inc4);

		tmps[3] = _mm_bsrli_si128(tmps[2], 8);
		tmps[2] = _mm_bslli_si128(tmps[2], 8);

		X = _mm_xor_si128(tmps[3], tmps[1]);
		Y = _mm_xor_si128(tmps[0], tmps[2]);
	}

	Y = polyreduce128(ctx, Y);
	Z = _mm_xor_si128(X, Y);
	X = Z;

	for (size_t i = 0; i < bx4rem; i++)
	{
		data[0] = _mm_loadu_si128((__m128i *)M + blen - bx4rem + i);
		tmps[0] = ctr[0];
		tmps[0] = aesenc128(tmps[0], ctx.aesctx.keys128);
		tmps[1] = _mm_xor_si128(tmps[0], data[0]);
		_mm_storeu_si128((__m128i *)C + blen - bx4rem + i, tmps[1]);
		X = _mm_xor_si128(X, byterev(tmps[1]));
		X = polydot128(ctx, X, ctx.htbl[0]);
		ctr[0] = _mm_add_epi32(ctr[0], inc);
	}

	if (Mrem > 0)
	{
		uint8_t padded[16];
		memset(padded, 0, 16);
		memcpy(padded, M + Mlen - Mrem, Mrem);
		alignas(16) __m128i lastblk = _mm_loadu_si128(((__m128i *)padded));
		tmps[0] = aesenc128(ctr[0], ctx.aesctx.keys128);
		lastblk = _mm_xor_si128(lastblk, tmps[0]);
		memset(((uint8_t *)&lastblk) + Mrem, 0, 16 - Mrem);
		memcpy(C + Mlen - Mrem, ((uint8_t *)&lastblk), Mrem);

		X = _mm_xor_si128(X, byterev(lastblk));
		X = polydot128(ctx, ctx.htbl[0], X);
	}

	uint8_t chunk[16];
	memset(chunk, 0, 16);
	((uint64_t*)(chunk))[1] = Alen*8;
	((uint64_t*)(chunk))[0] = Mlen*8;
	tmps[0] = _mm_loadu_si128((__m128i*)chunk);
    X = _mm_xor_si128(X, tmps[0]);
    X = polydot128(ctx, X, ctx.htbl[0]);
	return byterev(X);
}


static inline __m128i ctrhash128x8(aesgcm_context ctx, uint8_t *A, size_t Alen, __m128i S, uint8_t *M, size_t Mlen, uint8_t *C)
{

	alignas(16) __m128i X = ghash128x8(ctx, A, Alen, 0, 0);

	alignas(16) __m128i data[8];
	alignas(16) __m128i tmps[16];
	alignas(16) __m128i ctr[8];
	for (size_t i = 0; i < 8; i++)
	{
		ctr[i] = _mm_add_epi32(_mm_setr_epi32(0, 0, 0, (i + 1) << 24), S);
	}
	
	alignas(16) __m128i inc8 = _mm_setr_epi32(0, 0, 0, 0x08000000);
	alignas(16) __m128i inc = _mm_setr_epi32(0, 0, 0, 0x01000000);

	__m128i *ttmps = tmps + 12;

	size_t blen = Mlen / 16;
	size_t bx8len = blen / 8;
	size_t bx8rem = blen % 8;
	size_t Mrem = Mlen % 16;

	alignas(16) __m128i Y = _mm_setzero_si128(), Z = _mm_setzero_si128();

	for (size_t i = 0; i < bx8len; i++)
	{
		loadx8((__m128i *)M + 8 * i, data);

		tmps[0] = aesenc128(ctr[0], ctx.aesctx.keys128);
		tmps[1] = aesenc128(ctr[1], ctx.aesctx.keys128);
		tmps[2] = aesenc128(ctr[2], ctx.aesctx.keys128);
		tmps[3] = aesenc128(ctr[3], ctx.aesctx.keys128);
		tmps[4] = aesenc128(ctr[4], ctx.aesctx.keys128);
		tmps[5] = aesenc128(ctr[5], ctx.aesctx.keys128);
		tmps[6] = aesenc128(ctr[6], ctx.aesctx.keys128);
		tmps[7] = aesenc128(ctr[7], ctx.aesctx.keys128);
		xorx8_1wise(data, tmps, tmps);
		storex8((__m128i*)C + 8*i, tmps);

		Y = polyreduce128(ctx, Y);
		Z = _mm_xor_si128(X, Y);
		Z = _mm_xor_si128(Z, byterev(tmps[0]));
		schoolbook_initialadd128(byterev(tmps[7]), ctx.htbl[0], ttmps);
		schoolbook_add128(byterev(tmps[6]), ctx.htbl[1], ttmps);
		schoolbook_add128(byterev(tmps[5]), ctx.htbl[2], ttmps);
		schoolbook_add128(byterev(tmps[4]), ctx.htbl[3], ttmps);
		schoolbook_add128(byterev(tmps[3]), ctx.htbl[4], ttmps);
		schoolbook_add128(byterev(tmps[2]), ctx.htbl[5], ttmps);
		schoolbook_add128(byterev(tmps[1]), ctx.htbl[6], ttmps);
		schoolbook_add128(Z, ctx.htbl[7], ttmps);
		ttmps[3] = _mm_bsrli_si128(ttmps[2], 8);
		ttmps[2] = _mm_bslli_si128(ttmps[2], 8);
		X = _mm_xor_si128(ttmps[3], ttmps[1]);
		Y = _mm_xor_si128(ttmps[0], ttmps[2]);

		for (size_t j = 0; j < 8; j++)
		{
			ctr[j] = _mm_add_epi32(ctr[j], inc8);
		}
	}

	Y = polyreduce128(ctx, Y);
	Z = _mm_xor_si128(X, Y);
	X = Z;

	for (size_t i = 0; i < bx8rem; i++)
	{
		data[0] = _mm_loadu_si128((__m128i *)M + bx8len * 8 + i);
		tmps[0] = ctr[0];
		tmps[0] = aesenc128(tmps[0], ctx.aesctx.keys128);
		tmps[1] = _mm_xor_si128(tmps[0], data[0]);
		_mm_storeu_si128((__m128i *)C + bx8len * 8 + i, tmps[1]);
		X = _mm_xor_si128(X, byterev(tmps[1]));
		X = polydot128(ctx, X, ctx.htbl[0]);
		ctr[0] = _mm_add_epi32(ctr[0], inc);
	}

	if (Mrem > 0)
	{
		uint8_t padded[16];
		memset(padded, 0, 16);
		memcpy(padded, M + Mlen - Mrem, Mrem);
		alignas(16) __m128i lastblk = _mm_loadu_si128(((__m128i *)padded));
		tmps[0] = aesenc128(ctr[0], ctx.aesctx.keys128);
		lastblk = _mm_xor_si128(lastblk, tmps[0]);
		memset(((uint8_t *)&lastblk) + Mrem, 0, 16 - Mrem);
		memcpy(C + Mlen - Mrem, ((uint8_t *)&lastblk), Mrem);

		X = _mm_xor_si128(X, byterev(lastblk));
		X = polydot128(ctx, ctx.htbl[0], X);
	}

	uint8_t chunk[16];
	memset(chunk, 0, 16);
	((uint64_t*)(chunk))[1] = Alen * 8;
	((uint64_t*)(chunk))[0] = Mlen * 8;
	tmps[0] = _mm_loadu_si128((__m128i*)chunk);
    X = _mm_xor_si128(X, tmps[0]);
    X = polydot128(ctx, X, ctx.htbl[0]);

	return byterev(X);
}
