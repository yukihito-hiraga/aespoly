#pragma once
#include "../common/common.h"
#include "gctr.h"
#include "core.h"
#include "ctrhash.h"
#include "hash.h"

#define min_size(a, b) (a > b) ? b : a

void init_htbl128(aesgcm_context *ctx, __m128i H)
{
    //myprint_m128(polydot128(*ctx, ctx->poly, _mm_setr_epi32(0x1, 0, 0, 0)), "id");
    //myprint_m128(polydot128(*ctx, _mm_setr_epi32(0x1, 0, 0, 0), _mm_setr_epi32(0x1, 0, 0, 0)), "inv");

    ctx->htbl[0] = polydot128(*ctx, ctx->poly, H);
    // printf("%d ", 0);
    //myprint_m128(ctx->htbl[0], "htbl");
    for (size_t i = 1; i < 4; i++)
    {
        ctx->htbl[i] = polydot128(*ctx, ctx->htbl[i - 1], H);
        // printf("%ld ", i);
        //myprint_m128(ctx->htbl[i], "htbl");
    }
}

// For some primitive polynomial such that the degree of the second highest term is less than 64
static inline __m128i fmul128(__m128i poly, __m128i a, __m128i b)
{
    alignas(16) __m128i tmp0, tmp1, tmp2, tmp3, X, Y;
    tmp0 = _mm_clmulepi64_si128(a, b, 0x00);
    tmp1 = _mm_clmulepi64_si128(a, b, 0x11);
    tmp2 = _mm_clmulepi64_si128(a, b, 0x10);
    tmp3 = _mm_clmulepi64_si128(a, b, 0x01);
    tmp2 = _mm_xor_si128(tmp2, tmp3);

    X = _mm_xor_si128(tmp0, _mm_slli_si128(tmp2, 8));
    Y = _mm_xor_si128(tmp1, _mm_srli_si128(tmp2, 8));

    tmp0 = _mm_clmulepi64_si128(poly, Y, 0x00);
    tmp1 = _mm_clmulepi64_si128(poly, Y, 0x11);
    tmp2 = _mm_clmulepi64_si128(poly, Y, 0x10);
    tmp3 = _mm_clmulepi64_si128(poly, Y, 0x01);
    tmp2 = _mm_xor_si128(tmp2, tmp3);

    X = _mm_xor_si128(X, _mm_xor_si128(tmp0, _mm_slli_si128(tmp2, 8)));
    Y = _mm_xor_si128(tmp1, _mm_srli_si128(tmp2, 8));

    // then the degree of Y is less than 64

    X = _mm_xor_si128(X, _mm_clmulepi64_si128(poly, Y, 0x00));

    return X;
}

void aesgcminit128(aesgcm_context *ctx, uint8_t *key)
{
    aesinit128(&(ctx->aesctx), key);

    ctx->poly_original = _mm_setr_epi32(0x87, 0, 0, 0);
    ctx->poly = _mm_setr_epi32(1, 0, 0, 0xc2000000);

    alignas(16) __m128i H = _mm_setr_epi32(0, 0, 0, 0);
    H = aesenc128(H, ctx->aesctx.keys128);
    H = byterev(H);
    H = fmul128(ctx->poly, H, _mm_setr_epi32(2, 0, 0, 0));
    
    init_htbl128(ctx, H);
    
}

static inline __m128i gcm_j0_from_iv(aesgcm_context ctx, const uint8_t *IV, size_t IVlen)
{
    uint8_t chunk[16];
    memset(chunk, 0, 16);
    ((__uint64_t *)chunk)[0] = (uint64_t)IVlen * 8;
    __m128i X = ghash128x4(ctx, 0, 0, (uint8_t *)IV, IVlen);
    __m128i blk = _mm_loadu_si128((__m128i *)chunk);
    X = _mm_xor_si128(X, blk);
    X = polydot128(ctx, X, ctx.htbl[0]);
    return byterev(X);
}

static inline void aesgcm128x4(aesgcm_context ctx, uint8_t *IV, size_t IVlen, uint8_t *A, size_t Alen, uint8_t *P, size_t Plen, uint8_t *C, uint8_t *Tag, size_t Taglen)
{
    alignas(16) __m128i S;
    alignas(16) __m128i T, tmp;

    uint8_t chunk[16];
    memset(chunk, 0, 16);

    if (IVlen + 4 == 16)
    {
        uint8_t s[16];
		memset(s, 0, 16);
        memcpy(s, IV, 12);
        s[15] = 1;

        S = _mm_loadu_si128((__m128i *)s);
    }
    else
    {
        S = gcm_j0_from_iv(ctx, IV, IVlen);
    }
	
    T = ctrhash128x4(ctx, A, Alen, S, P, Plen, C);
    tmp = aesenc128(S, ctx.aesctx.keys128);
    T = _mm_xor_si128(tmp, T);

    memcpy(Tag, (uint8_t *)&T, min_size(Taglen, 16));
}

static inline void aesgcm128x8(aesgcm_context ctx, uint8_t *IV, size_t IVlen, uint8_t *A, size_t Alen, uint8_t *P, size_t Plen, uint8_t *C, uint8_t *Tag, size_t Taglen)
{
    alignas(16) __m128i S;
    alignas(16) __m128i T, tmp;

    uint8_t chunk[16];
    memset(chunk, 0, 16);

    if (IVlen + 4 == 16)
    {
        uint8_t s[16];
        memset(s, 0, 16);
        memcpy(s, IV, 12);
        s[15] = 1;

        S = _mm_loadu_si128((__m128i *)s);
    }
    else
    {
        S = gcm_j0_from_iv(ctx, IV, IVlen);
    }

    T = ctrhash128x8(ctx, A, Alen, S, P, Plen, C);
    tmp = aesenc128(S, ctx.aesctx.keys128);
    T = _mm_xor_si128(tmp, T);

    memcpy(Tag, (uint8_t *)&T, min_size(Taglen, 16));
}

static inline void aesgcm128_serialx4(aesgcm_context ctx, uint8_t *IV, size_t IVlen, uint8_t *A, size_t Alen, uint8_t *P, size_t Plen, uint8_t *C, uint8_t *Tag, size_t Taglen)
{
    alignas(16) __m128i S;
    alignas(16) __m128i T, tmp;

    uint8_t chunk[16];
    memset(chunk, 0, 16);

    if (IVlen + 4 == 16)
    {
        uint8_t s[16];
        memset(s, 0, 16);
        memcpy(s, IV, 12);
        s[15] = 1;

        S = _mm_loadu_si128((__m128i *)s);
    }
    else
    {
        S = gcm_j0_from_iv(ctx, IV, IVlen);
    }

    gctr128x4(ctx, S, P, Plen, C);
    T = ghash128x4(ctx, A, Alen, C, Plen);

	memset(chunk, 0, 16);

	((uint64_t *)(chunk + 0))[0] = Plen * 8;
    ((uint64_t *)(chunk + 8))[0] = Alen * 8;
	tmp = _mm_loadu_si128((__m128i*)chunk);
    T = _mm_xor_si128(T, tmp);
    T = polydot128(ctx, T, ctx.htbl[0]);
    T = byterev(T);

    tmp = aesenc128(S, ctx.aesctx.keys128);
    T = _mm_xor_si128(tmp, T);

    memcpy(Tag, (uint8_t *)&T, min_size(Taglen, 16));
}

static inline void aesgcm128_serialx8(aesgcm_context ctx, uint8_t *IV, size_t IVlen, uint8_t *A, size_t Alen, uint8_t *P, size_t Plen, uint8_t *C, uint8_t *Tag, size_t Taglen)
{
    alignas(16) __m128i S;
    alignas(16) __m128i T, tmp;

    uint8_t chunk[16];
    memset(chunk, 0, 16);

    if (IVlen + 4 == 16)
    {
        uint8_t s[16];
        memset(s, 0, 16);
        memcpy(s, IV, 12);
        s[15] = 1;

        S = _mm_loadu_si128((__m128i *)s);
    }
    else
    {
        S = gcm_j0_from_iv(ctx, IV, IVlen);
    }

    gctr128x8(ctx, S, P, Plen, C);
    T = ghash128x8(ctx, A, Alen, C, Plen);

	memset(chunk, 0, 16);

	((uint64_t*)(chunk+0))[0] = Plen * 8;
	((uint64_t*)(chunk+8))[0] = Alen * 8;
	tmp = _mm_loadu_si128((__m128i*)chunk);
    T = _mm_xor_si128(T, tmp);
    T = polydot128(ctx, T, ctx.htbl[0]);
    T = byterev(T);

    tmp = aesenc128(S, ctx.aesctx.keys128);
    T = _mm_xor_si128(tmp, T);

    memcpy(Tag, (uint8_t *)&T, min_size(Taglen, 16));
}
