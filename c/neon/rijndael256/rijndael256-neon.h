#pragma once
#include "../common/common.h"

typedef struct _rijndael256_context
{
	alignas(16) __m128i keys128[32];
	alignas(16) __m128i shuffle[2];
	alignas(16) __m128i mask[2];
} rijndael256_context;

static inline void init_rijndael256(rijndael256_context *ctx, uint8_t *key);

static inline void rijndael256_fix_128(rijndael256_context ctx, __m128i *pt, __m128i *out)
{
	alignas(16) __m128i tmps[2];

	tmps[0] = _mm_shuffle_epi8(pt[0], ctx.shuffle[0]);
	tmps[1] = _mm_shuffle_epi8(pt[1], ctx.shuffle[1]);

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
	tmps[0] = _mm_aesenc_si128(tmps[0], ctx.keys128[20]);
	tmps[1] = _mm_aesenc_si128(tmps[1], ctx.keys128[21]);

	rijndael256_fix_128(ctx, tmps, tmps);
	tmps[0] = _mm_aesenc_si128(tmps[0], ctx.keys128[22]);
	tmps[1] = _mm_aesenc_si128(tmps[1], ctx.keys128[23]);

	rijndael256_fix_128(ctx, tmps, tmps);
	tmps[0] = _mm_aesenc_si128(tmps[0], ctx.keys128[24]);
	tmps[1] = _mm_aesenc_si128(tmps[1], ctx.keys128[25]);

	rijndael256_fix_128(ctx, tmps, tmps);
	tmps[0] = _mm_aesenc_si128(tmps[0], ctx.keys128[26]);
	tmps[1] = _mm_aesenc_si128(tmps[1], ctx.keys128[27]);

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
		(tmps)[0] = _mm_aesenclast_si128((tmps)[0], (ctx.keys128)[28]); \
		(tmps)[1] = _mm_aesenclast_si128((tmps)[1], (ctx.keys128)[29]); \
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

static inline uint8_t rijndael256_sbox_u8(uint8_t x)
{
	static const uint8_t sbox[256] = {
		0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
		0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
		0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
		0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
		0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
		0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
		0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
		0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
		0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
		0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
		0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
		0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
		0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
		0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
		0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
		0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16};
	return sbox[x];
}

static inline __m128i rijndael256_seedvec(uint8_t w0, uint8_t w1, uint8_t w2, uint8_t w3)
{
	alignas(16) uint8_t seed[16] = {0};
	seed[12] = w0;
	seed[13] = w1;
	seed[14] = w2;
	seed[15] = w3;
	return _mm_loadu_si128((__m128i *)seed);
}

static inline __m128i rijndael256_expand_lo(__m128i lo, __m128i hi, uint8_t rcon)
{
	alignas(16) uint8_t hb[16];
	_mm_storeu_si128((__m128i *)hb, hi);
	__m128i seed = rijndael256_seedvec(
		rijndael256_sbox_u8(hb[13]) ^ rcon,
		rijndael256_sbox_u8(hb[14]),
		rijndael256_sbox_u8(hb[15]),
		rijndael256_sbox_u8(hb[12]));
	return shiftadd(lo, seed);
}

static inline __m128i rijndael256_expand_hi(__m128i lo, __m128i hi)
{
	alignas(16) uint8_t lb[16];
	_mm_storeu_si128((__m128i *)lb, lo);
	__m128i seed = rijndael256_seedvec(
		rijndael256_sbox_u8(lb[12]),
		rijndael256_sbox_u8(lb[13]),
		rijndael256_sbox_u8(lb[14]),
		rijndael256_sbox_u8(lb[15]));
	return shiftadd(hi, seed);
}

#define rijndael256_expand_assist1(lo, hi, rcon_imm) \
	do                                                \
	{                                                 \
		(lo) = rijndael256_expand_lo((lo), (hi), rcon_imm); \
	} while (0)

#define rijndael256_expand_assist2(lo, hi) \
	do                                      \
	{                                       \
		(hi) = rijndael256_expand_hi((lo), (hi)); \
	} while (0)

#define rijndael256_expand_pair(ctx, idx, lo, hi, rcon_imm)                               \
	do                                                                                     \
	{                                                                                      \
		rijndael256_expand_assist1((lo), (hi), rcon_imm);                                  \
		(ctx)->keys128[2 * (idx)] = (lo);                                                  \
		rijndael256_expand_assist2((lo), (hi));                                            \
		(ctx)->keys128[2 * (idx) + 1] = (hi);                                              \
	} while (0)

static inline void init_rijndael256(rijndael256_context *ctx, uint8_t *key)
{
	__m128i lo = _mm_loadu_si128((__m128i *)(key + 0));
	__m128i hi = _mm_loadu_si128((__m128i *)(key + 16));

	ctx->keys128[0] = lo;
	ctx->keys128[1] = hi;
	rijndael256_expand_pair(ctx, 1, lo, hi, 0x01);
	rijndael256_expand_pair(ctx, 2, lo, hi, 0x02);
	rijndael256_expand_pair(ctx, 3, lo, hi, 0x04);
	rijndael256_expand_pair(ctx, 4, lo, hi, 0x08);
	rijndael256_expand_pair(ctx, 5, lo, hi, 0x10);
	rijndael256_expand_pair(ctx, 6, lo, hi, 0x20);
	rijndael256_expand_pair(ctx, 7, lo, hi, 0x40);
	rijndael256_expand_pair(ctx, 8, lo, hi, 0x80);
	rijndael256_expand_pair(ctx, 9, lo, hi, 0x1B);
	rijndael256_expand_pair(ctx, 10, lo, hi, 0x36);
	rijndael256_expand_pair(ctx, 11, lo, hi, 0x6C);
	rijndael256_expand_pair(ctx, 12, lo, hi, 0xD8);
	rijndael256_expand_pair(ctx, 13, lo, hi, 0xAB);
	rijndael256_expand_pair(ctx, 14, lo, hi, 0x4D);

	ctx->shuffle[0] = _mm_setr_epi8(0, 1, 6, 7, 4, 5, 10, 11, 8, 9, 14, 15, 12, 13, 2, 3);
	ctx->shuffle[1] = ctx->shuffle[0];
	ctx->mask[0] = _mm_setr_epi8(0x00, (char)0x80, (char)0x80, (char)0x80, 0x00, 0x00, (char)0x80, (char)0x80,
	                             0x00, 0x00, 0x00, (char)0x80, 0x00, 0x00, (char)0x80, (char)0x80);
	ctx->mask[1] = ctx->mask[0];
}
