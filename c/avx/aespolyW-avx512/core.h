#pragma once
#include "../common/common.h"
#include <stdbool.h>

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

typedef struct _aespolyW_context
{
	aes_context aes_ctx;
	alignas(64) __m512i poly, poly_double;
	alignas(64) __m128i poly128, poly_double128;
	alignas(64) __m512i poly_quadruple1, poly_quadruple2;
	alignas(64) __m512i htbl[16];
	alignas(64) __m512i L[300];
	alignas(64) __m512i omega[300];
	alignas(64) __m512i offsetL[3000];
	alignas(64) __m512i offsetomega[3000];
} aespolyW_context;

static inline __m512i aeskeyex512(__m512i key, uint8_t rcon)
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
	aesctx->keys[1] = aeskeyex512(aesctx->keys[0], 0x01);
	aesctx->keys[2] = aeskeyex512(aesctx->keys[1], 0x02);
	aesctx->keys[3] = aeskeyex512(aesctx->keys[2], 0x04);
	aesctx->keys[4] = aeskeyex512(aesctx->keys[3], 0x08);
	aesctx->keys[5] = aeskeyex512(aesctx->keys[4], 0x10);
	aesctx->keys[6] = aeskeyex512(aesctx->keys[5], 0x20);
	aesctx->keys[7] = aeskeyex512(aesctx->keys[6], 0x40);
	aesctx->keys[8] = aeskeyex512(aesctx->keys[7], 0x80);
	aesctx->keys[9] = aeskeyex512(aesctx->keys[8], 0x1B);
	aesctx->keys[10] = aeskeyex512(aesctx->keys[9], 0x36);
}

static inline __m512i aesenc512(__m512i pt, __m512i *keys)
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

aespolyW_context global_ctx;