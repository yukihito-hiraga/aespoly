#pragma once
#include "../common/common.h"
#include "core.h"
#include "hash.h"
#include "xctr.h"
#include "xctrhash.h"

void init_htbl128(hctr2_context *ctx, __m128i *H)
{
    ctx->htbl[0] = polydot128(ctx->poly, H[0], _mm_setr_epi32(2, 0, 0, 0));
    ctx->htbl[4] = polydot128(ctx->poly, H[1], _mm_setr_epi32(2, 0, 0, 0));
    for (size_t i = 1; i < 4; i++)
    {
        ctx->htbl[i] = polydot128(ctx->poly, ctx->htbl[i - 1], H[0]);
        ctx->htbl[i + 4] = polydot128(ctx->poly, ctx->htbl[i + 4 - 1], H[1]);
    }
}

void hctr2init128(hctr2_context *ctx, uint8_t *key)
{
    ctx->poly = _mm_setr_epi32(0x1, 0, 0, 0xc2000000);

    init_rijndael256(&(ctx->rijndael256ctx), key);

    ctx->L[0] = _mm_setr_epi32(0, 0, 0, 0);
    ctx->L[1] = _mm_setr_epi32(1, 0, 0, 0);
    rijndael256x1(ctx->rijndael256ctx, ctx->L, ctx->L);

    alignas(16) __m128i H[2] = {_mm_setr_epi32(0, 0, 0, 0), _mm_setr_epi32(0, 0, 0, 0)};

    rijndael256x1(ctx->rijndael256ctx, H, H);
    init_htbl128(ctx, H);
}

static inline void hctr2enc128x4(hctr2_context ctx, uint8_t *T, size_t Tlen, uint8_t *P, size_t Mlen, uint8_t *C)
{
    size_t Nlen = (Mlen - 32) > 0 ? Mlen - 32 : 0;
    alignas(16) __m128i tmps[12];

    alignas(16) __m128i M[2];
    M[0] = _mm_loadu_si128((__m128i *)P);
    M[1] = _mm_loadu_si128((__m128i *)P + 1);
    uint8_t *N = P + 32;
    uint8_t *V = C + 32;

    __m128i *state = tmps;
    __m128i *hash = tmps + 2;
    __m128i *MM = tmps + 4;
    __m128i *UU = tmps + 6;
    __m128i *S = tmps + 8;
    __m128i *U = tmps + 10;

    tweakhash128n2x4(ctx, Nlen % 16 > 0, T, Tlen, state);

    hash128x4(ctx, state, N, Nlen, hash);

    MM[0] = _mm_xor_si128(M[0], hash[0]);
    MM[1] = _mm_xor_si128(M[1], hash[1]);

    UU[0] = MM[0];
    UU[1] = MM[1];

    rijndael256x1(ctx.rijndael256ctx, UU, UU);

    S[0] = _mm_xor_si128(_mm_xor_si128(ctx.L[0], MM[0]), UU[0]);
    S[1] = _mm_xor_si128(_mm_xor_si128(ctx.L[1], MM[1]), UU[1]);

    xctrxoradd_hash128x4(ctx, state, S, N, Nlen, V, hash);

    U[0] = _mm_xor_si128(UU[0], hash[0]);
    U[1] = _mm_xor_si128(UU[1], hash[1]);

    _mm_storeu_si128((__m128i *)C, U[0]);
    _mm_storeu_si128((__m128i *)C + 1, U[1]);
}

static inline void hctr2enc128x8(hctr2_context ctx, uint8_t *T, size_t Tlen, uint8_t *P, size_t Mlen, uint8_t *C)
{
    size_t Nlen = (Mlen - 32) > 0 ? Mlen - 32 : 0;
    alignas(16) __m128i tmps[12];

    alignas(16) __m128i M[2];
    M[0] = _mm_loadu_si128((__m128i *)P);
    M[1] = _mm_loadu_si128((__m128i *)P + 1);
    uint8_t *N = P + 32;
    uint8_t *V = C + 32;

    __m128i *state = tmps;
    __m128i *hash = tmps + 2;
    __m128i *MM = tmps + 4;
    __m128i *UU = tmps + 6;
    __m128i *S = tmps + 8;
    __m128i *U = tmps + 10;

    tweakhash128n2x8(ctx, Nlen % 16 > 0, T, Tlen, state);

    hash128x8(ctx, state, N, Nlen, hash);

    MM[0] = _mm_xor_si128(M[0], hash[0]);
    MM[1] = _mm_xor_si128(M[1], hash[1]);

    UU[0] = MM[0];
    UU[1] = MM[1];

    rijndael256x1(ctx.rijndael256ctx, UU, UU);

    S[0] = _mm_xor_si128(_mm_xor_si128(ctx.L[0], MM[0]), UU[0]);
    S[1] = _mm_xor_si128(_mm_xor_si128(ctx.L[1], MM[1]), UU[1]);

    xctrxoradd_hash128x8(ctx, state, S, N, Nlen, V, hash);

    U[0] = _mm_xor_si128(UU[0], hash[0]);
    U[1] = _mm_xor_si128(UU[1], hash[1]);

    _mm_storeu_si128((__m128i *)C, U[0]);
    _mm_storeu_si128((__m128i *)C + 1, U[1]);
}

void hctr2dec128(aes_context aesctx, hctr2_context ctx, uint8_t *T, size_t Tlen, uint8_t *C, size_t Clen, uint8_t *P)
{
    size_t Vlen = (Clen - 32) > 0 ? Clen - 32 : 0;
    alignas(16) __m128i tmps[12];

    alignas(16) __m128i U[2];
    U[0] = _mm_loadu_si128((__m128i *)C);
    U[1] = _mm_loadu_si128((__m128i *)C + 1);
    uint8_t *N = P + 32;
    uint8_t *V = C + 32;

    __m128i *state = tmps;
    __m128i *hash = tmps + 2;
    __m128i *UU = tmps + 4;
    __m128i *MM = tmps + 6;
    __m128i *S = tmps + 8;
    __m128i *M = tmps + 10;

    tweakhash128n2x4(ctx, Vlen % 16 > 0, T, Tlen, state);

    hash128x4(ctx, state, V, Vlen, hash);

    UU[0] = _mm_xor_si128(U[0], hash[0]);
    UU[1] = _mm_xor_si128(U[1], hash[1]);

    MM[0] = UU[0];
    MM[1] = UU[1];

    //this should be inverse
    rijndael256x1(ctx.rijndael256ctx, MM, MM);

    S[0] = _mm_xor_si128(_mm_xor_si128(ctx.L[0], MM[0]), UU[0]);
    S[1] = _mm_xor_si128(_mm_xor_si128(ctx.L[1], MM[1]), UU[1]);

    xctrxoradd_hash128x8(ctx, state, S, V, Vlen, N, hash);

    M[0] = _mm_xor_si128(MM[0], hash[0]);
    M[1] = _mm_xor_si128(MM[1], hash[1]);

    _mm_storeu_si128((__m128i *)P, M[0]);
    _mm_storeu_si128((__m128i *)P + 1, M[1]);
}