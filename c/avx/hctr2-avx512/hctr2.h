#pragma once
#include "../common/common.h"
#include "core.h"
#include "hash.h"
#include "xctr.h"
#include "xctrhash.h"

void init_htbl512(hctr2_context *ctx, __m128i H)
{
    ctx->hh = _mm512_broadcast_i64x2(H);
    alignas(512) __m512i T = ctx->hh;
    alignas(128) __m128i id = _mm_setr_epi32(0x1, 0, 0, 0xc2000000);
    ctx->htbl[0] = T;
    for (size_t i = 0; i < 3; i++)
    {
        ((__m128i *)&T)[3 - i] = id;
        ctx->htbl[0] = polydot512(ctx, ctx->htbl[0], T);
    }
    alignas(512) __m512i H4 = _mm512_broadcast_i64x2(((__m128i *)&ctx->htbl[0])[0]);
    ctx->hx4tbl[0] = H4;
    for (size_t i = 1; i < 4; i++)
    {
        ctx->htbl[i] = polydot512(ctx, ctx->htbl[i - 1], H4);
    }
    for (size_t i = 1; i < 4; i++)
    {
        ctx->hx4tbl[i] = polydot512(ctx, ctx->hx4tbl[i - 1], H4);
    }
}

void hctr2init512(aes_context *aesctx, hctr2_context *ctx, uint8_t *key)
{
    ctx->blocklength = 16;
    ctx->poly = _mm512_broadcast_i64x2(_mm_setr_epi32(0x1, 0, 0, 0xc2000000));
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

    alignas(512) __m512i H = aesenc512(_mm512_setzero_si512(), aesctx->keys);
    alignas(512) __m512i L = _mm512_broadcast_i64x2(_mm_setr_epi32(1, 0, 0, 0));
    L = aesenc512(L, aesctx->keys);
    init_htbl512(ctx, _mm512_castsi512_si128(H));
    ctx->L = L;
}

static inline void hctr2enc512(aes_context* aesctx, hctr2_context* ctx, uint8_t *P, size_t mlen, uint8_t *T, size_t tlen, uint8_t *C)
{
    size_t nlen = mlen - ctx->blocklength;
    alignas(512) __m512i L = ctx->L;
    alignas(512) __m512i tmp0, tmp1, TMP2;

    alignas(512) __m512i M = _mm512_broadcast_i64x2(_mm_loadu_si128((__m128i *)P));
    uint8_t *N = P + ctx->blocklength;
    uint8_t *V = C + ctx->blocklength;

    tmp0 = hash512(ctx, N, nlen, T, tlen);
    tmp0 = _mm512_xor_si512(M, tmp0);
    tmp1 = aesenc512(tmp0, aesctx->keys);
    tmp0 = _mm512_xor_si512(tmp0, tmp1);
    tmp0 = _mm512_xor_si512(tmp0, L);
    tmp0 = _mm512_broadcast_i64x2(((__m128i *)&tmp0)[0]);
    tmp0 = xctrxoradd_hash512(aesctx, ctx, tmp0, nlen, N, V, T, tlen);
    tmp0 = _mm512_xor_si512(tmp0, tmp1);
    _mm_storeu_si128((__m128i *)C, _mm512_castsi512_si128(tmp0));
}

static inline void hctr2enc512p(aes_context* aesctx, hctr2_context* ctx, uint8_t *P, size_t mlen, uint8_t *T, size_t tlen, uint8_t *C)
{
    size_t nlen = mlen - ctx->blocklength;
    alignas(512) __m512i L = ctx->L;
    alignas(512) __m512i tmp0, tmp1, TMP2;

    alignas(512) __m512i M = _mm512_broadcast_i64x2(_mm_loadu_si128((__m128i *)P));
    uint8_t *N = P + ctx->blocklength;
    uint8_t *V = C + ctx->blocklength;

    tmp0 = hash512(ctx, N, nlen, T, tlen);
    tmp0 = _mm512_xor_si512(M, tmp0);
    tmp1 = aesenc512(tmp0, aesctx->keys);
    tmp0 = _mm512_xor_si512(tmp0, tmp1);
    tmp0 = _mm512_xor_si512(tmp0, L);
    tmp0 = _mm512_broadcast_i64x2(((__m128i *)&tmp0)[0]);
    tmp0 = xctrxoradd_hash512(aesctx, ctx, tmp0, nlen, N, V, T, tlen);
    tmp0 = _mm512_xor_si512(tmp0, tmp1);
    _mm_storeu_si128((__m128i *)C, _mm512_castsi512_si128(tmp0));
}

void hctr2dec512(aes_context* aesctx, hctr2_context* ctx, uint8_t *C, size_t clen, uint8_t *T, size_t tlen, uint8_t *P)
{
    size_t vlen = clen - ctx->blocklength;
    alignas(512) __m512i L = ctx->L;
    alignas(512) __m512i tmp0, tmp1, TMP2;

    alignas(512) __m512i U = _mm512_broadcast_i64x2(_mm_loadu_si128((__m128i *)P));
    uint8_t *N = P + ctx->blocklength;
    uint8_t *V = C + ctx->blocklength;

    tmp0 = hash512(ctx, V, vlen, T, tlen);
    tmp0 = _mm512_xor_si512(U, tmp0);
    tmp1 = aesdec512(tmp0, aesctx->keys);
    tmp0 = _mm512_xor_si512(tmp0, tmp1);
    tmp0 = _mm512_xor_si512(tmp0, L);
    xctrxoradd512(aesctx, ctx, tmp0, vlen, V, N);
    tmp0 = hash512(ctx, N, vlen, T, tlen);
    tmp0 = _mm512_xor_si512(tmp0, tmp1);
    _mm_storeu_si128((__m128i *)P, _mm512_castsi512_si128(tmp0));
}
