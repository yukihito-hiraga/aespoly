#pragma once
#include "../common/common.h"

typedef struct _rijndael256_context
{
	alignas(16) __m128i keys128[32];
	alignas(16) __m128i shuffle[2];
	alignas(16) __m128i mask[2];
} rijndael256_context;

static inline void rijndael256_fix_128(rijndael256_context ctx, __m128i *pt, __m128i *out)
{
	alignas(16) __m128i tmps[6];
	__m128i *tmp1 = tmps + 2;

	tmps[0] = _mm_shuffle_epi8(pt[0], ctx.shuffle[0]);
	tmps[1] = _mm_shuffle_epi8(pt[1], ctx.shuffle[1]);

	// tmp1[0] = _mm_and_si128(tmps[0], ctx.mask[0]);
	// tmp1[1] = _mm_and_si128(tmps[1], ctx.mask[1]);
	// tmps[4] = _mm_xor_si128(tmp1[0], tmp1[1]);
	// out[0] = _mm_xor_si128(tmps[0], tmps[4]);
	// out[1] = _mm_xor_si128(tmps[1], tmps[4]);
	out[0] = _mm_blendv_epi8(tmps[0], tmps[1], ctx.mask[0]);
	out[1] = _mm_blendv_epi8(tmps[1], tmps[0], ctx.mask[0]);
}
static inline void rijndael256_128(rijndael256_context ctx, __m128i *pt, __m128i *ct)
{
	alignas(16) __m128i tmps[2];

	tmps[0] = _mm_xor_si128(pt[0], ctx.keys128[0]);
	tmps[1] = _mm_xor_si128(pt[1], ctx.keys128[1]);

	rijndael256_fix_128(ctx, tmps, tmps);
	tmps[0] = _mm_aesenc_si128(tmps[0], ctx.keys128[2]);
	tmps[1] = _mm_aesenc_si128(tmps[1], ctx.keys128[3]);

	rijndael256_fix_128(ctx, tmps, tmps);
	tmps[0] = _mm_aesenc_si128(tmps[0], ctx.keys128[4]);
	tmps[1] = _mm_aesenc_si128(tmps[1], ctx.keys128[5]);

	rijndael256_fix_128(ctx, tmps, tmps);
	tmps[0] = _mm_aesenc_si128(tmps[0], ctx.keys128[6]);
	tmps[1] = _mm_aesenc_si128(tmps[1], ctx.keys128[7]);

	rijndael256_fix_128(ctx, tmps, tmps);
	tmps[0] = _mm_aesenc_si128(tmps[0], ctx.keys128[8]);
	tmps[1] = _mm_aesenc_si128(tmps[1], ctx.keys128[9]);

	rijndael256_fix_128(ctx, tmps, tmps);
	tmps[0] = _mm_aesenc_si128(tmps[0], ctx.keys128[10]);
	tmps[1] = _mm_aesenc_si128(tmps[1], ctx.keys128[11]);

	rijndael256_fix_128(ctx, tmps, tmps);
	tmps[0] = _mm_aesenc_si128(tmps[0], ctx.keys128[12]);
	tmps[1] = _mm_aesenc_si128(tmps[1], ctx.keys128[13]);

	rijndael256_fix_128(ctx, tmps, tmps);
	tmps[0] = _mm_aesenc_si128(tmps[0], ctx.keys128[14]);
	tmps[1] = _mm_aesenc_si128(tmps[1], ctx.keys128[15]);

	rijndael256_fix_128(ctx, tmps, tmps);
	tmps[0] = _mm_aesenc_si128(tmps[0], ctx.keys128[16]);
	tmps[1] = _mm_aesenc_si128(tmps[1], ctx.keys128[17]);

	rijndael256_fix_128(ctx, tmps, tmps);
	tmps[0] = _mm_aesenc_si128(tmps[0], ctx.keys128[18]);
	tmps[1] = _mm_aesenc_si128(tmps[1], ctx.keys128[19]);

	rijndael256_fix_128(ctx, tmps, tmps);
	tmps[0] = _mm_aesenclast_si128(tmps[0], ctx.keys128[20]);
	tmps[1] = _mm_aesenclast_si128(tmps[1], ctx.keys128[21]);

	rijndael256_fix_128(ctx, tmps, tmps);
	tmps[0] = _mm_aesenclast_si128(tmps[0], ctx.keys128[22]);
	tmps[1] = _mm_aesenclast_si128(tmps[1], ctx.keys128[23]);

	rijndael256_fix_128(ctx, tmps, tmps);
	tmps[0] = _mm_aesenclast_si128(tmps[0], ctx.keys128[24]);
	tmps[1] = _mm_aesenclast_si128(tmps[1], ctx.keys128[25]);

	rijndael256_fix_128(ctx, tmps, tmps);
	tmps[0] = _mm_aesenclast_si128(tmps[0], ctx.keys128[26]);
	tmps[1] = _mm_aesenclast_si128(tmps[1], ctx.keys128[27]);

	rijndael256_fix_128(ctx, tmps, tmps);
	tmps[0] = _mm_aesenclast_si128(tmps[0], ctx.keys128[28]);
	tmps[1] = _mm_aesenclast_si128(tmps[1], ctx.keys128[29]);

	ct[0] = tmps[0];
	ct[1] = tmps[1];
}

static inline void rijndael256_ecb(rijndael256_context ctx, uint8_t *P, size_t Plen, uint8_t *C)
{
	alignas(16) __m128i tmps[4];
	size_t remainder = Plen % (2 * 16);
	size_t ln = Plen / (2 * 16);

	for (size_t i = 0; i < ln; i++)
	{
		loadx2((__m128i *)P + 2 * i, tmps);

		rijndael256_128(ctx, tmps, tmps);

		storex2((__m128i *)C + 2 * i, tmps);
	}
}

#define rijndael256_rounds(ctx, rijndael256_round_f, tmps) \
	{                                                      \
		rijndael256_round_f(ctx, 1, tmps);                 \
		rijndael256_round_f(ctx, 2, tmps);                 \
		rijndael256_round_f(ctx, 3, tmps);                 \
		rijndael256_round_f(ctx, 4, tmps);                 \
		rijndael256_round_f(ctx, 5, tmps);                 \
		rijndael256_round_f(ctx, 6, tmps);                 \
		rijndael256_round_f(ctx, 7, tmps);                 \
		rijndael256_round_f(ctx, 8, tmps);                 \
		rijndael256_round_f(ctx, 9, tmps);                 \
		rijndael256_round_f(ctx, 10, tmps);                 \
		rijndael256_round_f(ctx, 11, tmps);                 \
		rijndael256_round_f(ctx, 12, tmps);                 \
		rijndael256_round_f(ctx, 13, tmps);                 \
	}

#define rijndael256_round(ctx, i, tmps)                                    \
	{                                                                      \
		rijndael256_fix_128(ctx, tmps, tmps);                              \
		(tmps)[0] = _mm_aesenc_si128((tmps)[0], (ctx.keys128)[0 + 2 * i]); \
		(tmps)[1] = _mm_aesenc_si128((tmps)[1], (ctx.keys128)[1 + 2 * i]); \
	}

#define rijndael256_lastround(ctx, tmps)                                \
	{                                                                   \
		rijndael256_fix_128(ctx, tmps, tmps);                           \
		(tmps)[0] = _mm_aesenclast_si128((tmps)[0], (ctx.keys128)[20]); \
		(tmps)[1] = _mm_aesenclast_si128((tmps)[1], (ctx.keys128)[21]); \
	}

#define rijndael256_roundx2(ctx, i, tmps)    \
	{                                        \
		rijndael256_round(ctx, i, tmps);     \
		rijndael256_round(ctx, i, tmps + 2); \
	}

#define rijndael256_roundx4(ctx, i, tmps)      \
	{                                          \
		rijndael256_roundx2(ctx, i, tmps);     \
		rijndael256_roundx2(ctx, i, tmps + 4); \
	}

#define rijndael256_roundx8(ctx, i, tmps)      \
	{                                          \
		rijndael256_roundx4(ctx, i, tmps);     \
		rijndael256_roundx4(ctx, i, tmps + 8); \
	}

#define rijndael256_roundx16(ctx, i, tmps)      \
	{                                           \
		rijndael256_roundx8(ctx, i, tmps);      \
		rijndael256_roundx8(ctx, i, tmps + 16); \
	}

#define rijndael256_roundx32(ctx, i, tmps)       \
	{                                            \
		rijndael256_roundx16(ctx, i, tmps);      \
		rijndael256_roundx16(ctx, i, tmps + 32); \
	}

#define rijndael256_roundx64(ctx, i, tmps)       \
	{                                            \
		rijndael256_roundx32(ctx, i, tmps);      \
		rijndael256_roundx32(ctx, i, tmps + 64); \
	}

#define rijndael256_lastroundx2(ctx, tmps)    \
	{                                         \
		rijndael256_lastround(ctx, tmps);     \
		rijndael256_lastround(ctx, tmps + 2); \
	}

#define rijndael256_lastroundx4(ctx, tmps)      \
	{                                           \
		rijndael256_lastroundx2(ctx, tmps);     \
		rijndael256_lastroundx2(ctx, tmps + 4); \
	}

#define rijndael256_lastroundx8(ctx, tmps)      \
	{                                           \
		rijndael256_lastroundx4(ctx, tmps);     \
		rijndael256_lastroundx4(ctx, tmps + 8); \
	}

#define rijndael256_lastroundx16(ctx, tmps)      \
	{                                            \
		rijndael256_lastroundx8(ctx, tmps);      \
		rijndael256_lastroundx8(ctx, tmps + 16); \
	}

#define rijndael256_lastroundx32(ctx, tmps)       \
	{                                             \
		rijndael256_lastroundx16(ctx, tmps);      \
		rijndael256_lastroundx16(ctx, tmps + 32); \
	}

#define rijndael256_lastroundx64(ctx, tmps)       \
	{                                             \
		rijndael256_lastroundx32(ctx, tmps);      \
		rijndael256_lastroundx32(ctx, tmps + 64); \
	}

#define rijndael256x1(ctx, pt, tmps)                      \
	{                                                     \
		addkey256(ctx.keys128, pt, tmps);                 \
		rijndael256_rounds(ctx, rijndael256_round, tmps); \
		rijndael256_lastround(ctx, tmps);                 \
	}

#define rijndael256x2(ctx, pt, tmps)                        \
	{                                                       \
		addkey256x2(ctx.keys128, pt, tmps);                 \
		rijndael256_rounds(ctx, rijndael256_roundx2, tmps); \
		rijndael256_lastroundx2(ctx, tmps);                 \
	}

#define rijndael256x4(ctx, pt, tmps)                        \
	{                                                       \
		addkey256x4(ctx.keys128, pt, tmps);                 \
		rijndael256_rounds(ctx, rijndael256_roundx4, tmps); \
		rijndael256_lastroundx4(ctx, tmps);                 \
	}

#define rijndael256x8(ctx, pt, tmps)                        \
	{                                                       \
		addkey256x8(ctx.keys128, pt, tmps);                 \
		rijndael256_rounds(ctx, rijndael256_roundx8, tmps); \
		rijndael256_lastroundx8(ctx, tmps);                 \
	}

#define rijndael256x16(ctx, pt, tmps)                        \
	{                                                        \
		addkey256x16(ctx.keys128, pt, tmps);                 \
		rijndael256_rounds(ctx, rijndael256_roundx16, tmps); \
		rijndael256_lastroundx16(ctx, tmps);                 \
	}

#define rijndael256x32(ctx, pt, tmps)                        \
	{                                                        \
		addkey256x32(ctx.keys128, pt, tmps);                 \
		rijndael256_rounds(ctx, rijndael256_roundx32, tmps); \
		rijndael256_lastroundx32(ctx, tmps);                 \
	}

#define rijndael256x64(ctx, pt, tmps)                        \
	{                                                        \
		addkey256x64(ctx.keys128, pt, tmps);                 \
		rijndael256_rounds(ctx, rijndael256_roundx64, tmps); \
		rijndael256_lastroundx64(ctx, tmps);                 \
	}

static inline void rijndael256_ecbx16(rijndael256_context ctx, uint8_t *P, size_t Plen, uint8_t *C)
{
	alignas(16) __m128i tmps[32];
	size_t remainder = Plen % (32 * 16);
	size_t ln = Plen / (32 * 16);

	for (size_t i = 0; i < ln; i++)
	{
		loadx32((__m128i *)P + 32 * i, tmps);

		rijndael256x16(ctx, tmps, tmps);

		storex32((__m128i *)C + 32 * i, tmps);
	}
}

static inline void rijndael256_ecbx32(rijndael256_context ctx, uint8_t *P, size_t Plen, uint8_t *C)
{
	alignas(16) __m128i tmps[64];
	size_t remainder = Plen % (64 * 16);
	size_t ln = Plen / (64 * 16);

	for (size_t i = 0; i < ln; i++)
	{
		loadx64((__m128i *)P + 64 * i, tmps);

		rijndael256x32(ctx, tmps, tmps);

		storex64((__m128i *)C + 64 * i, tmps);
	}
}

static inline void rijndael256xctrx16(rijndael256_context ctx, __m128i *S, uint8_t *P, size_t Plen, uint8_t *C)
{
	size_t blen = Plen / 16;
	size_t dlen = blen / 2;
	size_t dx16len = dlen / 16;
	size_t dx16rem = dlen % 16;

	alignas(16) __m128i tmps[32];
	alignas(16) __m128i data[32];

	alignas(16) __m128i inc = _mm_setr_epi32(1, 0, 0, 0);
	alignas(16) __m128i inc16 = _mm_setr_epi32(16, 0, 0, 0);

	alignas(16) __m128i ctr[32] = {
		_mm_xor_si128(_mm_setr_epi32(1, 0, 0, 0), S[0]),
		_mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S[1]),
		_mm_xor_si128(_mm_setr_epi32(2, 0, 0, 0), S[0]),
		_mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S[1]),
		_mm_xor_si128(_mm_setr_epi32(3, 0, 0, 0), S[0]),
		_mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S[1]),
		_mm_xor_si128(_mm_setr_epi32(4, 0, 0, 0), S[0]),
		_mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S[1]),
		_mm_xor_si128(_mm_setr_epi32(5, 0, 0, 0), S[0]),
		_mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S[1]),
		_mm_xor_si128(_mm_setr_epi32(6, 0, 0, 0), S[0]),
		_mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S[1]),
		_mm_xor_si128(_mm_setr_epi32(7, 0, 0, 0), S[0]),
		_mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S[1]),
		_mm_xor_si128(_mm_setr_epi32(8, 0, 0, 0), S[0]),
		_mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S[1]),

		_mm_xor_si128(_mm_setr_epi32(9, 0, 0, 0), S[0]),
		_mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S[1]),
		_mm_xor_si128(_mm_setr_epi32(10, 0, 0, 0), S[0]),
		_mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S[1]),
		_mm_xor_si128(_mm_setr_epi32(11, 0, 0, 0), S[0]),
		_mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S[1]),
		_mm_xor_si128(_mm_setr_epi32(12, 0, 0, 0), S[0]),
		_mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S[1]),
		_mm_xor_si128(_mm_setr_epi32(13, 0, 0, 0), S[0]),
		_mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S[1]),
		_mm_xor_si128(_mm_setr_epi32(14, 0, 0, 0), S[0]),
		_mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S[1]),
		_mm_xor_si128(_mm_setr_epi32(15, 0, 0, 0), S[0]),
		_mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S[1]),
		_mm_xor_si128(_mm_setr_epi32(16, 0, 0, 0), S[0]),
		_mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S[1]),
	};

	for (size_t i = 0; i < dx16len; i++)
	{
		loadx32((__m128i *)P + 32 * i, data);

		rijndael256x16(ctx, ctr, tmps);
		xorx32_1wise(tmps, data, tmps);

		storex32((__m128i *)C + 32 * i, tmps);
		addx16_bfix_2wise(ctr, inc16, ctr);
	}
	for (size_t i = 0; i < dx16rem; i++)
	{
		loadx2((__m128i *)P + 32 * dx16len + 2 * i, data);
		rijndael256x1(ctx, ctr, tmps);
		xorx2_1wise(tmps, data, tmps);
		storex2((__m128i *)C + 32 * dx16len + 2 * i, tmps);
		addx1_bfix_2wise(ctr, inc, ctr);
	}
}

static inline void rijndael256xctrx4(rijndael256_context ctx, __m128i *S, uint8_t *P, size_t Plen, uint8_t *C)
{
	size_t blen = Plen / 16;
	size_t dlen = blen / 2;
	size_t dx4len = dlen / 4;
	size_t dx4rem = dlen % 4;

	alignas(16) __m128i tmps[32];
	alignas(16) __m128i data[32];

	alignas(16) __m128i inc = _mm_setr_epi32(1, 0, 0, 0);
	alignas(16) __m128i inc4 = _mm_setr_epi32(4, 0, 0, 0);

	alignas(16) __m128i ctr[8] = {
		_mm_xor_si128(_mm_setr_epi32(1, 0, 0, 0), S[0]),
		_mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S[1]),
		_mm_xor_si128(_mm_setr_epi32(2, 0, 0, 0), S[0]),
		_mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S[1]),
		_mm_xor_si128(_mm_setr_epi32(3, 0, 0, 0), S[0]),
		_mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S[1]),
		_mm_xor_si128(_mm_setr_epi32(4, 0, 0, 0), S[0]),
		_mm_xor_si128(_mm_setr_epi32(0, 0, 0, 0), S[1]),
	};

	for (size_t i = 0; i < dx4len; i++)
	{
		loadx8((__m128i *)P + 8 * i, data);

		rijndael256x4(ctx, ctr, tmps);
		xorx8_1wise(tmps, data, tmps);

		storex8((__m128i *)C + 8 * i, tmps);
		addx4_bfix_2wise(ctr, inc4, ctr);
	}
	for (size_t i = 0; i < dx4rem; i++)
	{
		loadx2((__m128i *)P + 8 * dx4len + 2 * i, data);
		rijndael256x1(ctx, ctr, tmps);
		xorx2_1wise(tmps, data, tmps);
		storex2((__m128i *)C + 8 * dx4len + 2 * i, tmps);
		addx1_bfix_2wise(ctr, inc, ctr);
	}
}

void init_rijndael256(rijndael256_context* ctx, uint8_t*key){
	ctx->keys128[0] = _mm_loadu_si128((__m128i*)key);
	ctx->keys128[1] = _mm_loadu_si128((__m128i*)key+1);
}