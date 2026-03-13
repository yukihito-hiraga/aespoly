#pragma once
#include "headers.h"

typedef struct _aes_context
{
	alignas(64) __m512i keys[12];
	alignas(32) __m256i keys256[12];
	alignas(16) __m128i keys128[12];
} aes_context;

static inline __m128i aesdec128(__m128i ct, __m128i *keys)
{
	alignas(16) __m128i tmp;
	tmp = _mm_xor_si128(ct, keys[10]);
	tmp = _mm_aesdec_si128(tmp, keys[9]);
	tmp = _mm_aesdec_si128(tmp, keys[8]);
	tmp = _mm_aesdec_si128(tmp, keys[7]);
	tmp = _mm_aesdec_si128(tmp, keys[6]);
	tmp = _mm_aesdec_si128(tmp, keys[5]);
	tmp = _mm_aesdec_si128(tmp, keys[4]);
	tmp = _mm_aesdec_si128(tmp, keys[3]);
	tmp = _mm_aesdec_si128(tmp, keys[2]);
	tmp = _mm_aesdec_si128(tmp, keys[1]);
	tmp = _mm_aesdeclast_si128(tmp, keys[0]);
	return tmp;
}

static inline __m128i aeskeyex128(__m128i key, uint8_t rcon)
{
	alignas(128) __m128i tmpkey = key;
	alignas(128) __m128i tmp = _mm_aeskeygenassist_si128(key, rcon);
	tmp = _mm_shuffle_epi32(tmp, 0xff);
	tmpkey = _mm_xor_si128(tmpkey, _mm_slli_si128(tmpkey, 4));
	tmpkey = _mm_xor_si128(tmpkey, _mm_slli_si128(tmpkey, 4));
	tmpkey = _mm_xor_si128(tmpkey, _mm_slli_si128(tmpkey, 4));
	tmpkey = _mm_xor_si128(tmpkey, tmp);
	return tmpkey;
}

static inline void aesinit128(aes_context *aesctx, uint8_t *key)
{
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
}

inline __m128i aesenc128(__m128i pt, __m128i *keys)
{
	alignas(16) __m128i tmp;
	tmp = _mm_xor_si128(pt, keys[0]);
	tmp = _mm_aesenc_si128(tmp, keys[1]);
	tmp = _mm_aesenc_si128(tmp, keys[2]);
	tmp = _mm_aesenc_si128(tmp, keys[3]);
	tmp = _mm_aesenc_si128(tmp, keys[4]);
	tmp = _mm_aesenc_si128(tmp, keys[5]);
	tmp = _mm_aesenc_si128(tmp, keys[6]);
	tmp = _mm_aesenc_si128(tmp, keys[7]);
	tmp = _mm_aesenc_si128(tmp, keys[8]);
	tmp = _mm_aesenc_si128(tmp, keys[9]);
	tmp = _mm_aesenclast_si128(tmp, keys[10]);
	return tmp;
}

static inline __m256i aesdec256(__m256i ct, __m256i *keys)
{
	alignas(32) __m256i tmp;
	tmp = _mm256_xor_si256(ct, keys[10]);
	tmp = _mm256_aesdec_epi128(tmp, keys[9]);
	tmp = _mm256_aesdec_epi128(tmp, keys[8]);
	tmp = _mm256_aesdec_epi128(tmp, keys[7]);
	tmp = _mm256_aesdec_epi128(tmp, keys[6]);
	tmp = _mm256_aesdec_epi128(tmp, keys[5]);
	tmp = _mm256_aesdec_epi128(tmp, keys[4]);
	tmp = _mm256_aesdec_epi128(tmp, keys[3]);
	tmp = _mm256_aesdec_epi128(tmp, keys[2]);
	tmp = _mm256_aesdec_epi128(tmp, keys[1]);
	tmp = _mm256_aesdeclast_epi128(tmp, keys[0]);
	return tmp;
}

static inline __m256i aeskeyex256(__m256i key, uint8_t rcon)
{
	alignas(128) __m128i key128 = _mm256_castsi256_si128(key);
	alignas(128) __m128i tmp = _mm_aeskeygenassist_si128(_mm256_castsi256_si128(key), rcon);
	tmp = _mm_shuffle_epi32(tmp, 0xff);
	key128 = _mm_xor_si128(key128, _mm_slli_si128(key128, 4));
	key128 = _mm_xor_si128(key128, _mm_slli_si128(key128, 4));
	key128 = _mm_xor_si128(key128, _mm_slli_si128(key128, 4));
	key128 = _mm_xor_si128(key128, tmp);
	return _mm256_broadcast_i64x2(key128);
}

static inline void aesinit256(aes_context *aesctx, uint8_t *key)
{
	aesctx->keys256[0] = _mm256_broadcast_i64x2(_mm_loadu_si128((__m128i *)key));
	aesctx->keys256[1] = aeskeyex256(aesctx->keys256[0], 0x01);
	aesctx->keys256[2] = aeskeyex256(aesctx->keys256[1], 0x02);
	aesctx->keys256[3] = aeskeyex256(aesctx->keys256[2], 0x04);
	aesctx->keys256[4] = aeskeyex256(aesctx->keys256[3], 0x08);
	aesctx->keys256[5] = aeskeyex256(aesctx->keys256[4], 0x10);
	aesctx->keys256[6] = aeskeyex256(aesctx->keys256[5], 0x20);
	aesctx->keys256[7] = aeskeyex256(aesctx->keys256[6], 0x40);
	aesctx->keys256[8] = aeskeyex256(aesctx->keys256[7], 0x80);
	aesctx->keys256[9] = aeskeyex256(aesctx->keys256[8], 0x1B);
	aesctx->keys256[10] = aeskeyex256(aesctx->keys256[9], 0x36);
}

inline __m256i aesenc256(__m256i pt, __m256i *keys)
{
	alignas(32) __m256i tmp;
	tmp = _mm256_xor_si256(pt, keys[0]);
	tmp = _mm256_aesenc_epi128(tmp, keys[1]);
	tmp = _mm256_aesenc_epi128(tmp, keys[2]);
	tmp = _mm256_aesenc_epi128(tmp, keys[3]);
	tmp = _mm256_aesenc_epi128(tmp, keys[4]);
	tmp = _mm256_aesenc_epi128(tmp, keys[5]);
	tmp = _mm256_aesenc_epi128(tmp, keys[6]);
	tmp = _mm256_aesenc_epi128(tmp, keys[7]);
	tmp = _mm256_aesenc_epi128(tmp, keys[8]);
	tmp = _mm256_aesenc_epi128(tmp, keys[9]);
	tmp = _mm256_aesenclast_epi128(tmp, keys[10]);
	return tmp;
}

#define aes_roundx1(key, src, dst)                  \
	{                                               \
		(dst)[0] = _mm_aesenc_si128((src)[0], key); \
	}

#define aes_lastroundx1(key, src, dst)                  \
	{                                                   \
		(dst)[0] = _mm_aesenclast_si128((src)[0], key); \
	}

#define aes_addkeyx1(key, src, dst)              \
	{                                            \
		(dst)[0] = _mm_xor_si128((src)[0], key); \
	}

#define aes_fx2(f, key, src, dst) \
	{                             \
		f(key, src, dst);         \
		f(key, src + 1, dst + 1); \
	}

#define aes_fx4(f, key, src, dst)          \
	{                                      \
		aes_fx2(f, key, src, dst);         \
		aes_fx2(f, key, src + 2, dst + 2); \
	}

#define aes_fx8(f, key, src, dst)          \
	{                                      \
		aes_fx4(f, key, src, dst);         \
		aes_fx4(f, key, src + 4, dst + 4); \
	}

#define aes_fx16(f, key, dst, src)         \
	{                                      \
		aes_fx8(f, key, src, dst);         \
		aes_fx8(f, key, src + 8, dst + 8); \
	}

#define aes_roundx2(key, src, dst) aes_fx2(aes_roundx1, key, src, dst)

#define aes_lastroundx2(key, src, dst) aes_fx2(aes_lastroundx1, key, src, dst)

#define aes_addkeyx2(key, src, dst) aes_fx2(aes_addkeyx1, key, src, dst)

#define aes_roundx4(key, src, dst) aes_fx4(aes_roundx1, key, src, dst)

#define aes_lastroundx4(key, src, dst) aes_fx4(aes_lastroundx1, key, src, dst)

#define aes_addkeyx4(key, src, dst) aes_fx4(aes_addkeyx1, key, src, dst)

#define aes_roundx8(key, src, dst) aes_fx8(aes_roundx1, key, src, dst)

#define aes_lastroundx8(key, src, dst) aes_fx8(aes_lastroundx1, key, src, dst)

#define aes_addkeyx8(key, src, dst) aes_fx8(aes_addkeyx1, key, src, dst)

#define aes_roundx16(key, src, dst) aes_fx16(aes_roundx1, key, src, dst)

#define aes_lastroundx16(key, src, dst) aes_fx16(aes_lastroundx1, key, src, dst)

#define aes_addkeyx16(key, src, dst) aes_fx16(aes_addkeyx1, key, src, dst)

#define aes_allrounds(aes_addkey_f, aes_round_f, aes_lastround_f, keys, src, dst) \
	{                                                                             \
		aes_addkey_f(keys[0], src, dst);                                          \
		aes_round_f(keys[1], dst, dst);                                           \
		aes_round_f(keys[2], dst, dst);                                           \
		aes_round_f(keys[3], dst, dst);                                           \
		aes_round_f(keys[4], dst, dst);                                           \
		aes_round_f(keys[5], dst, dst);                                           \
		aes_round_f(keys[6], dst, dst);                                           \
		aes_round_f(keys[7], dst, dst);                                           \
		aes_round_f(keys[8], dst, dst);                                           \
		aes_round_f(keys[9], dst, dst);                                           \
		aes_lastround_f(keys[10], dst, dst);                                      \
	}

#define aesx1(keys, src, dst) aes_allrounds(aes_addkeyx1, aes_roundx1, aes_lastroundx1, keys, src, dst)

#define aesx2(keys, src, dst) aes_allrounds(aes_addkeyx2, aes_roundx2, aes_lastroundx2, keys, src, dst)

#define aesx4(keys, src, dst) aes_allrounds(aes_addkeyx4, aes_roundx4, aes_lastroundx4, keys, src, dst)

#define aesx8(keys, src, dst) aes_allrounds(aes_addkeyx8, aes_roundx8, aes_lastroundx8, keys, src, dst)

#define aesx16(keys, src, dst) aes_allrounds(aes_addkeyx16, aes_roundx16, aes_lastroundx16, keys, src, dst)