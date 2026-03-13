#pragma once
#include "../common/common.h"
#include <stddef.h>

#define schoolbook_add512(tmps, reg, htbl_reg)                   \
    {                                                            \
        tmps[3] = _mm512_clmulepi64_epi128(reg, htbl_reg, 0x01); \
        tmps[2] = _mm512_xor_si512(tmps[2], tmps[3]);            \
        tmps[3] = _mm512_clmulepi64_epi128(reg, htbl_reg, 0x00); \
        tmps[0] = _mm512_xor_si512(tmps[0], tmps[3]);            \
        tmps[3] = _mm512_clmulepi64_epi128(reg, htbl_reg, 0x11); \
        tmps[1] = _mm512_xor_si512(tmps[1], tmps[3]);            \
        tmps[3] = _mm512_clmulepi64_epi128(reg, htbl_reg, 0x10); \
        tmps[2] = _mm512_xor_si512(tmps[2], tmps[3]);            \
    }

#define schoolbook_initialadd512(tmps, reg, htbl_reg)            \
    {                                                            \
        tmps[2] = _mm512_clmulepi64_epi128(reg, htbl_reg, 0x01); \
        tmps[0] = _mm512_clmulepi64_epi128(reg, htbl_reg, 0x00); \
        tmps[3] = _mm512_clmulepi64_epi128(reg, htbl_reg, 0x10); \
        tmps[2] = _mm512_xor_si512(tmps[2], tmps[3]);            \
        tmps[1] = _mm512_clmulepi64_epi128(reg, htbl_reg, 0x11); \
    }

static inline __m512i aeskeyex(__m512i key, uint8_t rcon)
{
	alignas(128) __m128i key128 = _mm512_castsi512_si128(key);
	alignas(128) __m128i tmp = _mm_aeskeygenassist_si128(_mm512_castsi512_si128(key), rcon);
	tmp = _mm_shuffle_epi32(tmp, 0xff);
	key128 = _mm_xor_si128(key128, _mm_slli_si128(key128, 4));
	key128 = _mm_xor_si128(key128, _mm_slli_si128(key128, 4));
	key128 = _mm_xor_si128(key128, _mm_slli_si128(key128, 4));
	key128 = _mm_xor_si128(key128, tmp);
	return _mm512_broadcast_i64x2(key128);
}

static inline void aesinit512(aes_context *aesctx, uint8_t *key)
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
}

inline __m512i aesenc512(__m512i pt, __m512i *keys)
{
	alignas(64) __m512i tmp;
	tmp = _mm512_xor_si512(pt, keys[0]);
	tmp = _mm512_aesenc_epi128(tmp, keys[1]);
	tmp = _mm512_aesenc_epi128(tmp, keys[2]);
	tmp = _mm512_aesenc_epi128(tmp, keys[3]);
	tmp = _mm512_aesenc_epi128(tmp, keys[4]);
	tmp = _mm512_aesenc_epi128(tmp, keys[5]);
	tmp = _mm512_aesenc_epi128(tmp, keys[6]);
	tmp = _mm512_aesenc_epi128(tmp, keys[7]);
	tmp = _mm512_aesenc_epi128(tmp, keys[8]);
	tmp = _mm512_aesenc_epi128(tmp, keys[9]);
	tmp = _mm512_aesenclast_epi128(tmp, keys[10]);
	return tmp;
}

static inline __m512i aesdec512(__m512i ct, __m512i *keys)
{
	alignas(512) __m512i tmp;
	tmp = _mm512_xor_si512(ct, keys[10]);
	tmp = _mm512_aesdec_epi128(tmp, keys[9]);
	tmp = _mm512_aesdec_epi128(tmp, keys[8]);
	tmp = _mm512_aesdec_epi128(tmp, keys[7]);
	tmp = _mm512_aesdec_epi128(tmp, keys[6]);
	tmp = _mm512_aesdec_epi128(tmp, keys[5]);
	tmp = _mm512_aesdec_epi128(tmp, keys[4]);
	tmp = _mm512_aesdec_epi128(tmp, keys[3]);
	tmp = _mm512_aesdec_epi128(tmp, keys[2]);
	tmp = _mm512_aesdec_epi128(tmp, keys[1]);
	tmp = _mm512_aesdeclast_epi128(tmp, keys[0]);
	return tmp;
}

static inline __m512i aesenc512_var(__m512i pt, __m512i key0, __m512i key1, __m512i key2, __m512i key3, __m512i key4, __m512i key5, __m512i key6, __m512i key7, __m512i key8, __m512i key9, __m512i key10)
{
	alignas(512) __m512i tmp;
	tmp = _mm512_xor_si512(pt, key0);
	tmp = _mm512_aesenc_epi128(tmp, key1);
	tmp = _mm512_aesenc_epi128(tmp, key2);
	tmp = _mm512_aesenc_epi128(tmp, key3);
	tmp = _mm512_aesenc_epi128(tmp, key4);
	tmp = _mm512_aesenc_epi128(tmp, key5);
	tmp = _mm512_aesenc_epi128(tmp, key6);
	tmp = _mm512_aesenc_epi128(tmp, key7);
	tmp = _mm512_aesenc_epi128(tmp, key8);
	tmp = _mm512_aesenc_epi128(tmp, key9);
	tmp = _mm512_aesenclast_epi128(tmp, key10);
	return tmp;
}

#define CAST2M512i(x) _mm512_setr_epi64((uint64_t)(x), 0, 0, 0, 0, 0, 0, 0)

typedef struct _hctr2_context
{
    size_t blocklength;
    alignas(64) __m512i poly;
    alignas(64) __m512i htbl[4];
    alignas(64) __m512i hx4tbl[4];
    alignas(64) __m512i hh;
    alignas(64) __m512i L;
} hctr2_context;