#pragma once
#include "../common/common.h"
#include "core.h"
#include "hash.h"
#include "xctr.h"
#include "xctrhash.h"

void init_htbl128(hctr2_context *ctx, __m128i H)
{
    ctx->htbl[0] = _mm_setr_epi32(0x1, 0, 0, 0xc2000000);
    for (size_t i = 1; i < 5; i++)
    {
        ctx->htbl[i] = polydot128(*ctx, ctx->htbl[i - 1], H);
    }
}

void hctr2init128(aes_context *aesctx, hctr2_context *ctx, uint8_t *key)
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

    alignas(16) __m128i H = _mm_setr_epi32(0, 0, 0, 0);
    alignas(16) __m128i L = _mm_setr_epi32(1, 0, 0, 0);
    L = aesenc128(L, aesctx->keys128);
    H = aesenc128(H, aesctx->keys128);
    //myprint_m128(H, "H");
    //myprint_m128(L, "L");
    init_htbl128(ctx, H);
    ctx->L = L;
}

static inline void hctr2enc128(aes_context aesctx, hctr2_context ctx, uint8_t *P, size_t mlen, uint8_t *T, size_t tlen, uint8_t *C)
{
    size_t nlen = mlen - ctx.blocklength;
    alignas(16) __m128i L = ctx.L;
    alignas(16) __m128i tmp0, tmp1;

    alignas(16) __m128i M = _mm_loadu_si128((__m128i *)P);
    uint8_t *N = P + ctx.blocklength;
    uint8_t *V = C + ctx.blocklength;

    tmp0 = hash128x8(ctx, N, nlen, T, tlen);
    tmp0 = _mm_xor_si128(M, tmp0);
    tmp1 = aesenc128(tmp0, aesctx.keys128);
    tmp0 = _mm_xor_si128(tmp0, tmp1);
    tmp0 = _mm_xor_si128(tmp0, L);
    xctrxoradd128(aesctx, ctx, tmp0, nlen, N, V);
    tmp0 = hash128x8(ctx, V, nlen, T, tlen);
    tmp0 = _mm_xor_si128(tmp0, tmp1);
    _mm_storeu_si128((__m128i *)C, tmp0);
}

static inline void hctr2enc128p_x4(aes_context aesctx, hctr2_context ctx, uint8_t *P, size_t mlen, uint8_t *T, size_t tlen, uint8_t *C)
{
    size_t nlen = mlen - ctx.blocklength;
    alignas(16) __m128i L = ctx.L;
    alignas(16) __m128i tmp0, tmp1;

    alignas(16) __m128i M = _mm_loadu_si128((__m128i *)P);
    uint8_t *N = P + ctx.blocklength;
    uint8_t *V = C + ctx.blocklength;

    tmp0 = hash128x4(ctx, N, nlen, T, tlen);
    tmp0 = _mm_xor_si128(M, tmp0);
    tmp1 = aesenc128(tmp0, aesctx.keys128);
    tmp0 = _mm_xor_si128(tmp0, tmp1);
    tmp0 = _mm_xor_si128(tmp0, L);
    tmp0 = xctrxoradd_hash128x4(aesctx, ctx, tmp0, nlen, N, V, T, tlen);
    tmp0 = _mm_xor_si128(tmp0, tmp1);
    _mm_storeu_si128((__m128i *)C, tmp0);
}

static inline void hctr2enc128p_x8(aes_context aesctx, hctr2_context ctx, uint8_t *P, size_t mlen, uint8_t *T, size_t tlen, uint8_t *C)
{
    size_t nlen = mlen - ctx.blocklength;
    alignas(16) __m128i L = ctx.L;
    alignas(16) __m128i tmp0, tmp1;

    alignas(16) __m128i M = _mm_loadu_si128((__m128i *)P);
    uint8_t *N = P + ctx.blocklength;
    uint8_t *V = C + ctx.blocklength;

    tmp0 = hash128x8(ctx, N, nlen, T, tlen);
    tmp0 = _mm_xor_si128(M, tmp0);
    tmp1 = aesenc128(tmp0, aesctx.keys128);
    tmp0 = _mm_xor_si128(tmp0, tmp1);
    tmp0 = _mm_xor_si128(tmp0, L);
    tmp0 = xctrxoradd_hash128x8(aesctx, ctx, tmp0, nlen, N, V, T, tlen);
    tmp0 = _mm_xor_si128(tmp0, tmp1);
    _mm_storeu_si128((__m128i *)C, tmp0);
}

void hctr2dec128(aes_context aesctx, hctr2_context ctx, uint8_t *C, size_t clen, uint8_t *T, size_t tlen, uint8_t *P)
{
    size_t vlen = clen - ctx.blocklength;
    alignas(16) __m128i L = ctx.L;
    alignas(16) __m128i tmp0, tmp1;

    alignas(16) __m128i U = _mm_loadu_si128((__m128i *)P);
    uint8_t *N = P + ctx.blocklength;
    uint8_t *V = C + ctx.blocklength;

    tmp0 = hash128(ctx, V, vlen, T, tlen);
    tmp0 = _mm_xor_si128(U, tmp0);
    tmp1 = aesdec128(tmp0, aesctx.keys128);
    tmp0 = _mm_xor_si128(tmp0, tmp1);
    tmp0 = _mm_xor_si128(tmp0, L);
    xctrxoradd128(aesctx, ctx, tmp0, vlen, V, N);
    tmp0 = hash128(ctx, N, vlen, T, tlen);
    tmp0 = _mm_xor_si128(tmp0, tmp1);
    _mm_storeu_si128((__m128i *)P, tmp0);
}